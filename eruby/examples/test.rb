require "eruby"

file = ARGV.shift || STDIN
compiler = ERuby::Compiler.new
print compiler.compile_file(file)
