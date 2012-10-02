def gen_operand_reader(operand_type)
  case operand_type
  when "imm", "str"
    %{printf(" ( %s )", sl_to_cstr(vm, sl_inspect(vm, NEXT_IMM())));}
  when "reg"
    %{printf(" ~%zu", NEXT_UINT());}
  when "uint"
    %{printf(" %zu", NEXT_UINT());}
  when "section"
    %{section_queue[++section_i] = NEXT_PTR();  printf(" <section %s (%p)>", sl_to_cstr(vm, section_queue[section_i]->name), section_queue[section_i]);}
  else
    raise "Unknown operand type: #{operand_type}"
  end
end
ARGF.readlines.slice_before { |line| line =~ %r{/\*\s+0: [A-Z_]+} }.each do |lines|
  ins, *operands = lines.take_while { |line| line !~ /^INSTRUCTION/ }
  ins =~ /0: ([A-Z_]+)/
  ins = $1
  operands = operands.map { |o| o =~ /([a-z]+)[:>]/; $1 }
  puts "case SL_OP_#{ins}:"
  puts %{    printf("    %04zu:  %-16s", ip, #{ins.inspect});}
  if operands.empty?
    puts '    printf("\n");'
    next
  end
  puts operands.map { |op| "    #{gen_operand_reader(op)}\n" }.join(%{    printf(",");\n})
  puts '    printf("\n");'
  puts '    break;'
end