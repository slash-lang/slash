-- =============================================================================

require('scripts/txt-to-c')

function error( msg ) print( 'Error: ' .. msg ) os.exit() end
function warning( msg ) print( 'Warning: ' .. msg ) end
function success( msg ) print( 'Success: ' .. msg ) end

function get_options( dispatch )
    for i,v in ipairs(_ARGS) do
        local id = v:sub(1, v:find('=') - 1)
        if dispatch[id] then
            dispatch[id](v:sub(v:find('=') + 1, -1))
        elseif dispatch['default'] then
            dispatch['default'](id, v:sub(v:find('=') + 1, -1))
        else
            error(string.format('Unable to dispatch \'%s\'', id))
        end
    end
end

-- =============================================================================

function action_configure()
    local _extensions = {}
    local _sapis = {}
    local _idirs = {}
    local _ldirs = {}
    local _slash = require('slash')

    -- Handle command line options:
    get_options({
        ['ext'] = function( ext )
            if os.isdir('ext/' .. ext) then table.insert(_extensions, ext) else error('Unknown extension \'' .. ext .. '\'.') end
        end,

        ['no-ext'] = function( ext )
            for i, v in ipairs(_extensions) do
                if v == ext then table.remove(_extensions, i)
                    return
                end
            end
        end,

        ['sapi'] = function( sapi )
            if os.isdir('sapi/' .. sapi) then table.insert(_sapis, sapi) else error('Unknown SAPI \'' .. sapi .. '\'.') end
        end,
    })

    -- Check for overrides:
    local options = {}

    -- Include directories
    for lib, path in pairs(_slash[os.get()].idirs) do
        options[lib .. '-idir'] = function( idir )
            _idirs[lib] = idir
        end
    end

    -- Library directories
    for lib, path in pairs(_slash[os.get()].ldirs) do
        options[lib .. '-ldir'] = function( ldir )
            _ldirs[lib] = ldir
        end
    end

    for i, v in ipairs(_extensions) do
        local extension = require('ext/' .. v .. '/config')

        -- Include directories
        for lib, path in pairs(extension[os.get()].idirs) do
            options[lib .. '-idir'] = function( idir )
                _idirs[lib] = idir
            end
        end

        -- Library directories
        for lib, path in pairs(extension[os.get()].ldirs) do
            options[lib .. '-ldir'] = function( ldir )
                _ldirs[lib] = ldir
            end
        end
    end

    for i, v in ipairs(_extensions) do
        local extension = require('ext/' .. v .. '/config')

        -- Include directories
        for lib, path in pairs(extension[os.get()].idirs) do
            options[lib .. '-idir'] = function( idir )
                _idirs[lib] = idir
            end
        end

        -- Library directories
        for lib, path in pairs(extension[os.get()].ldirs) do
            options[lib .. '-ldir'] = function( ldir )
                _ldirs[lib] = ldir
            end
        end
    end

    for i, v in ipairs(_sapis) do
        local sapi = require('sapi/' .. v .. '/config')

        -- Include directories
        for lib, path in pairs(sapi[os.get()].idirs) do
            options[lib .. '-idir'] = function( idir )
                _idirs[lib] = idir
            end
        end

        -- Library directories
        for lib, path in pairs(sapi[os.get()].ldirs) do
            options[lib .. '-ldir'] = function( ldir )
                _ldirs[lib] = ldir
            end
        end
    end

    -- Generate config.lua:
    local config_file = assert(io.open('config.lua', 'w'))

    config_file:write('_extensions = {\n')
    for i, v in ipairs(_extensions) do config_file:write(string.format('  \'%s\',\n', tostring(v))) end
    config_file:write('}\n\n')

    config_file:write('_sapis = {\n')
    for i, v in ipairs(_sapis) do config_file:write(string.format('  \'%s\',\n', tostring(v))) end
    config_file:write('}\n\n')

    config_file:write('_idirs = {\n')
    for k, v in pairs(_idirs) do config_file:write(string.format('  [\'%s\'] = \'%s\',\n', tostring(k), tostring(v))) end
    config_file:write('}\n\n')

    config_file:write('_ldirs = {\n')
    for k, v in pairs(_ldirs) do config_file:write(string.format('  [\'%s\'] = \'%s\',\n', tostring(k), tostring(v))) end
    config_file:write('}')

    config_file:close()

    success('Generated config.lua!')
end

function action_install()
    error('Action \'install\' not implemented yet.')
end

function action_uninstall()
    error('Action \'uninstall\' not implemented yet.')
end

function action_build( action )
    require('config')
    local _slash = require('slash')

    solution('slash')
        location(string.format('build/%s', os.get()))
        configurations({'debug', 'release'})

    project('slash')
        kind('StaticLib')
        language('C')
        objdir('build/' .. os.get() .. '/slash/obj' )
        targetdir('lib/' .. os.get() .. '/')
        targetname('slash')

        configuration('debug') targetsuffix('-dbg') flags({'Symbols'}) defines({'SL_DEBUG'})
        configuration('release') flags({'Optimize', 'EnableSSE', 'EnableSSE2'})

        configuration({})
            local platform_compat = {
                ['windows'] = { 'platform/win32.c' },
                ['macosx']  = { 'platform/posix.c', 'platform/darwin.c' },
                ['linux']   = { 'platform/posix.c', 'platform/linux.c' },
            }

            if not platform_compat[os.get()] then error(string.format('Unsupported platform %s.', os.get()))
            else files(platform_compat[os.get()]) end

            os.outputof('flex -o src/lex.c src/lex.yy')
            txt_to_c('sl__error_page_src', 'src/lib/error_page.sl', 'src/lib/error_page.c')

            includedirs({ 'inc' })
            files(_slash[os.get()].files)
            links(_slash[os.get()].libs)

            for lib, path in pairs(_slash[os.get()].idirs) do
                if _idirs[lib] then includedirs(_idirs[lib])
                else includedirs(path) end
            end

            for lib, path in pairs(_slash[os.get()].ldirs) do
                if _ldirs[lib] then libdirs(_ldirs[lib])
                else libdirs(path) end
            end

        local ext_decls = {}
        local ext_inits = {}
        local ext_static_inits = {}

        for i, v in ipairs(_extensions) do
            print(string.format('Adding \'%s\' (extension)...', v))

            local extension = require('ext/' .. v .. '/config')

            files(extension[os.get()].files)
            links(extension[os.get()].libs)

            table.insert(ext_decls, string.format('void sl_init_ext_%s(sl_vm_t* vm);', v))
            table.insert(ext_inits, string.format('sl_init_ext_%s(vm);', v))

            if extension.needs_static_init then
                table.insert(ext_decls, string.format('void sl_static_init_ext_%s();', v))
                table.insert(ext_inits, string.format('sl_static_init_ext_%s();', v))
            end

            for lib, path in pairs(extension[os.get()].idirs) do
                if _idirs[lib] then includedirs(_idirs[lib])
                else includedirs(path) end
            end

            for lib, path in pairs(extension[os.get()].ldirs) do
                if _ldirs[lib] then libdirs(_ldirs[lib])
                else libdirs(path) end
            end
        end

        -- Generate src/init_exts.c:
        print('Generating src/init_exts.c...')

        local init_exts_file = assert(io.open('src/init_exts.c', 'w'))

        init_exts_file:write('#include "slash/slash.h"\n\n')

        for i,v in ipairs(ext_decls) do
            init_exts_file:write(string.format('%s\n\n', v))
        end

        init_exts_file:write('void\nsl_init_exts(sl_vm_t* vm)\n{\n')
        for i,v in ipairs(ext_inits) do init_exts_file:write(string.format('%s\n', v)) end
        init_exts_file:write('}\n\n')

        init_exts_file:write('void\nsl_static_init_exts()\n{\n')
        for i,v in ipairs(ext_static_inits) do init_exts_file:write(string.format('%s\n', v)) end
        init_exts_file:write('}\n\n')

        init_exts_file:close()

        files({'src/init_exts.c'})

    for i, v in ipairs(_sapis) do
        print(string.format('Adding \'%s\' (SAPI)...', v))

        project(v)
            kind('ConsoleApp')
            language('C')
            objdir('build/' .. os.get() .. '/sapi/' .. v .. '/obj' )
            targetdir('bin/' .. os.get() .. '/')
            targetname('slash-' .. v)

        configuration('debug') targetsuffix('-dbg') flags({'Symbols'}) defines({'SL_DEBUG'})
        configuration('release') flags({'Optimize', 'EnableSSE', 'EnableSSE2'})

        configuration({})
            local sapi = require('sapi/' .. v .. '/config')

            includedirs({ 'inc' })
            files(sapi[os.get()].files)
            links(sapi[os.get()].libs)
            links('slash')

            for lib, path in pairs(sapi[os.get()].idirs) do
                if _idirs[lib] then includedirs(_idirs[lib])
                else includedirs(path) end
            end

            for lib, path in pairs(sapi[os.get()].ldirs) do
                if _ldirs[lib] then libdirs(_ldirs[lib])
                else libdirs(path) end
            end

            -- Automagically link in what slash requires
            links(_slash[os.get()].libs)
            
            for lib, path in pairs(_slash[os.get()].idirs) do
                if _idirs[lib] then includedirs(_idirs[lib])
                else includedirs(path) end
            end

            for lib, path in pairs(_slash[os.get()].ldirs) do
                if _ldirs[lib] then libdirs(_ldirs[lib])
                else libdirs(path) end
            end
    end

    success(string.format('Generated build files for %s successfully!', os.get()))
end

-- =============================================================================

local _action_dispatch = {
    ['configure'] = action_configure,
    ['install'] = action_install,
    ['uninstall'] = action_uninstall,
    ['gmake']  = function() action_build('gmake')  end,
    ['vs2005'] = function() error('The MSVC toolchain not supported.') end,
    ['vs2008'] = function() error('The MSVC toolchain not supported.') end,
    ['vs2010'] = function() error('The MSVC toolchain not supported.') end
}

newaction({
    trigger     = 'configure',
    description = 'Configure extensions and slash apis as well as overrides'
})

if _ACTION then
    if not _action_dispatch[_ACTION] then
        error('Unknown or unsupported action \'' .. _ACTION .. '\'.')
    else
        print('Dispatching \'' .. _ACTION .. '\'...')
        _action_dispatch[_ACTION]()
    end
end