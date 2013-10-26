def gen_operand_reader(operand_type)
  case operand_type
  when "imm"
    %{printf(" ( %s )", sl_to_cstr(vm, sl_inspect(vm, NEXT_IMM())));}
  when "id"
    %{printf(" :%s", sl_to_cstr(vm, sl_id_to_string(vm, NEXT_ID())));}
  when "imc"
    %{imc = NEXT_IMC(); printf(" #%s /%d", sl_to_cstr(vm, sl_id_to_string(vm, imc->id)), imc->argc);}
  when "icc"
    %{(void)NEXT_ICC(); printf(" <icc>");}
  when "reg"
    %{printf(" r%zu", NEXT_UINT());}
  when "uint32"
    %{printf(" %zu", NEXT_UINT());}
  when "uint16"
    %{printf(" %zu", NEXT_UINT());}
  when "section"
    %{section_queue[++section_i] = NEXT_SECTION();  printf(" <section %s (%p)>", sl_to_cstr(vm, sl_id_to_string(vm, section_queue[section_i]->name)), section_queue[section_i]);}
  else
    raise "Unknown operand type: #{operand_type}"
  end
end
ARGF.readlines.slice_before { |line| line =~ %r{/\*\s+0: [A-Z_]+} }.each do |lines|
  ins, *operands = lines.take_while { |line| line !~ /^INSTRUCTION/ }
  ins =~ /0: ([A-Z_]+)/
  ins = $1
  operands = operands.map { |o| o =~ /([a-z][a-z0-9]*)[:>]/; $1 }
  puts "case SL_OP_#{ins}:"
  puts %{    printf("    %04zu:  %-16s", orig_ip, #{ins.inspect});}
  if operands.empty?
    puts '    printf("\n");'
    puts '    break;'
    next
  end
  puts operands.map { |op| "    #{gen_operand_reader(op)}\n" }.join(%{    printf(",");\n})
  puts '    printf("\n");'
  puts '    break;'
end
