class Operand
  attr_reader :type, :name

  def initialize(type, name)
    @type, @name = type, name
  end

  LOAD_MAP = {
    "REG"    => "NEXT_REG()",
    "REG*"   => "&NEXT_REG()",
    "size_t" => "NEXT_UINT()",
    "IP"     => "NEXT_UINT()",
    "SLID"   => "NEXT_ID()",
    "SLVAL"  => "NEXT_IMM()",
    "sl_vm_section_t*"               => "NEXT_SECTION()",
    "sl_vm_inline_method_cache_t*"   => "NEXT_IMC()",
    "sl_vm_inline_constant_cache_t*" => "NEXT_ICC()",
  }

  def code
    "#{c_type} #{name} = #{load_code};"
  end

  def c_type
    case type
    when "REG"
      "SLVAL"
    when "REG*"
      "SLVAL*"
    when "IP"
      "size_t"
    else
      type
    end
  end

  def load_code
    LOAD_MAP.fetch(type)
  end
end

class Instruction
  attr_reader :name, :operands, :code

  def initialize(name, operands, code)
    @name, @operands, @code = name, operands, code
  end

  def self.parse(str)
    first, *rest = str.lines.to_a

    unless /\A([A-Z_]+)\((.*)\)\Z/ =~ first
      raise "Syntax error on line: #{first.inspect}"
    end

    name, args = $1, $2

    operands = args.split(",").map { |arg|
      arg.strip.split
    }.map { |type, operand_name|
      Operand.new(type, operand_name)
    }

    new(name, operands, rest.join)
  end
end

source = File.read("src/vm_defn.inc")

# remove C-style /*...*/ comments
source.gsub!(%r{/\* .*? \*/}x, "")

# remove C++-style // comments
source.gsub!(%r{//.*$}, "")

# parse out instructions
insns = source.split(/(?=^[A-Z_]+\()/).reject { |section|
  section.empty?
}.map { |section|
  Instruction.parse(section)
}

Dir.mkdir("src/gen") unless Dir.exist?("src/gen")

# generate the opcode enum
File.open("src/gen/opcode_enum.inc", "w") do |f|
  f.puts "enum {"
  insns.each_with_index do |insn, index|
    f.printf "    SL_OP_%-16s = %2d,\n", insn.name, index
  end
  f.puts
  f.puts "    SL_OP__MAX_OPCODE"
  f.puts "};"
end

# generate a switch-driven vm
File.open("src/gen/switch_vm.inc", "w") do |f|
  insns.each do |insn|
    f.puts "case SL_OP_#{insn.name}: {"
    insn.operands.each do |operand|
      f.puts operand.code
    end
    f.puts insn.code
    f.puts "saved_ip = ip + 1;"
    f.puts "} break;"
    f.puts
  end
end

# generate a computed goto vm
File.open("src/gen/goto_vm.inc", "w") do |f|
  goto_code = <<-C
    #ifdef __GNUC__
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored \"-Wpedantic\"
    #endif
    goto *NEXT_THREADED_OPCODE();
    #ifdef __GNUC__
      #pragma GCC diagnostic pop
    #endif
  C
  f.puts goto_code
  insns.each do |insn|
    f.puts "vm_op_#{insn.name}: {"
    insn.operands.each do |operand|
      f.puts operand.code
    end
    f.puts insn.code
    f.puts "saved_ip = ip + 1;"
    f.puts goto_code
    f.puts "}"
    f.puts
  end
end

# generate the setup code for the computed goto vm
File.open("src/gen/goto_vm_setup.inc", "w") do |f|
  f.puts "#ifdef __GNUC__"
  f.puts "  #pragma GCC diagnostic push"
  f.puts "  #pragma GCC diagnostic ignored \"-Wpedantic\""
  f.puts "#endif"

  insns.each do |insn|
    f.puts "sl_vm_op_addresses[SL_OP_#{insn.name}] = &&vm_op_#{insn.name};"
  end

  f.puts "#ifdef __GNUC__"
  f.puts "  #pragma GCC diagnostic pop"
  f.puts "#endif"
end

# generate emitters for each opcode
File.open("src/gen/compile_helper.inc", "w") do |f|
  insns.each do |insn|
    args = "sl_compile_state_t* cs"
    insn.operands.each do |operand|
      c_type =
        case operand.type
        when "REG"  then "size_t"
        when "REG*" then "size_t"
        when "IP"   then "label_t*"
        else operand.type
        end
      args << ", #{c_type} #{operand.name}"
    end
    f.puts "static size_t op_#{insn.name.downcase}(#{args})"
    f.puts "{"
    f.puts "    sl_vm_insn_t insn;"
    f.puts "    size_t _ip = _emit_opcode(cs, SL_OP_#{insn.name});"
    insn.operands.each do |operand|
      if operand.type == "IP"
        f.puts "    _emit_label_operand(cs, #{operand.name});"
        next
      end
      field =
        case operand.type
        when "REG"    then "uint"
        when "REG*"   then "uint"
        when "SLVAL"  then "imm"
        when "size_t" then "uint"
        when "SLID"   then "id"
        when "sl_vm_section_t*" then "section"
        when "sl_vm_inline_method_cache_t*" then "imc"
        when "sl_vm_inline_constant_cache_t*" then "icc"
        else
          raise "dunno how to emit #{operand.type}"
        end
      f.puts "    insn.#{field} = #{operand.name};"
      f.puts "    _emit(cs, insn);"
    end
    f.puts "    return _ip;"
    f.puts "    (void)insn;"
    f.puts "}"
  end
end

# generate disassembler
File.open("sapi/disasm/dis.inc", "w") do |f|
  insns.each do |insn|
    f.puts "case SL_OP_#{insn.name}:"
    f.puts %{    printf("    %04zu:  %-16s", orig_ip, #{insn.name.inspect});}
    operand_disassemblers = insn.operands.map { |operand|
      case operand.type
      when "REG", "REG*"
        %{printf(" r%zu", NEXT_UINT());}
      when "SLVAL"
        %{printf(" %s", sl_to_cstr(vm, sl_inspect(vm, NEXT_IMM())));}
      when "size_t"
        %{printf(" %zu", NEXT_UINT());}
      when "IP"
        %{printf(" %zu", NEXT_UINT());}
      when "SLID"
        %{printf(" :%s", sl_to_cstr(vm, sl_id_to_string(vm, NEXT_ID())));}
      when "sl_vm_section_t*"
        %{section_queue[++section_i] = NEXT_SECTION();  printf(" <section %s (%p)>", sl_to_cstr(vm, sl_id_to_string(vm, section_queue[section_i]->name)), section_queue[section_i]);}
      when "sl_vm_inline_method_cache_t*"
        %{imc = NEXT_IMC(); printf(" <imc :%s (%d)>", sl_to_cstr(vm, sl_id_to_string(vm, imc->id)), imc->argc);}
      when "sl_vm_inline_constant_cache_t*"
        %{(void)NEXT_ICC(); printf(" <icc>");}
      end
    }
    f.puts operand_disassemblers.map { |code| "    #{code}\n" }.join(%{    printf(",");\n})
    f.puts '    printf("\n");'
    f.puts '    break;'
  end
end
