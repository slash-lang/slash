class Operand
  attr_reader :type, :name

  def initialize(type, name)
    @type, @name = type, name
  end

  LOAD_MAP = {
    "REG"    => "NEXT_REG()",
    "REG*"   => "&NEXT_REG()",
    "size_t" => "NEXT_UINT()",
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

    unless /\A(?<name>[A-Z_]+)\((?<args>.*)\)\Z/ =~ first
      raise "Syntax error on line: #{first.inspect}"
    end

    operands = args.split(",").map { |arg|
      arg.strip.split
    }.map { |type, name|
      Operand.new(type, name)
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
insns = source.lines.slice_before { |line|
  line =~ /^[A-Z_]+\(/
}.map { |lines|
  lines.join.strip
}.reject { |section|
  section.empty?
}.map { |section|
  Instruction.parse(section)
}

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
    f.puts "} break;"
    f.puts
  end
end

# generate a computed goto vm
File.open("src/gen/goto_vm.inc", "w") do |f|
  insns.each do |insn|
    f.puts "vm_op_#{insn.name}: {"
    insn.operands.each do |operand|
      f.puts operand.code
    end
    f.puts insn.code
    f.puts "}"
    f.puts "goto *NEXT_THREADED_OPCODE();"
    f.puts
  end
end

# generate the setup code for the computed goto vm
File.open("src/gen/goto_vm_setup.inc", "w") do |f|
  insns.each do |insn|
    f.puts "sl_vm_op_addresses[SL_OP_#{insn.name}] = &&vm_op_#{insn.name};"
  end
end
