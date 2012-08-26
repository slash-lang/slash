function txt_to_c( array_name, in_path, out_path )
    fout = assert(io.open(out_path, 'w'))
    fout:write('const char* ' .. array_name .. ' = ')
    for line in io.lines(in_path) do fout:write(string.format('"%s\\n"\n', line:gsub('"', '\\"'))) end
    fout:write(';\n')
    fout:close()
end