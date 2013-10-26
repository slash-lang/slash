class Operand
  attr_reader :type, :name

  def initialize(type, name)
    @type, @name = type, name
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
  lines.join
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
