# PowerShell script for building & testing SQLite3-Ruby fat binary gem
# Code by MSP-Greg, see https://github.com/MSP-Greg/av-gem-build-test

# load utility functions, pass 64 or 32
. $PSScriptRoot\shared\appveyor_setup.ps1 $args[0]
if ($LastExitCode) { exit }

# above is required code
#———————————————————————————————————————————————————————————————— above for all repos

Make-Const gem_name  'clipper'
Make-Const repo_name 'rbclipper'
Make-Const url_repo  'https://github.com/mieko/rbclipper.git'

#———————————————————————————————————————————————————————————————— lowest ruby version
Make-Const ruby_vers_low 20

#———————————————————————————————————————————————————————————————— make info
Make-Const dest_so  'lib'
Make-Const exts     @( @{ 'conf' = 'ext/clipper/extconf.rb' ; 'so' = 'clipper' } )
Make-Const write_so_require $false

#———————————————————————————————————————————————————————————————— Run-Tests
function Run-Tests {
  Update-Gems minitest, rake
  rake -f Rakefile_wintest -N -R norakelib | Set-Content -Path $log_name -PassThru -Encoding UTF8
  # add info after test results
  $(ruby -rsqlite3 -e "STDOUT.write $/ + 'SQLite3::SQLITE_VERSION ' ; puts SQLite3::SQLITE_VERSION") |
    Add-Content -Path $log_name -PassThru -Encoding UTF8
  minitest
}

#———————————————————————————————————————————————————————————————— below for all repos
# below is required code
Make-Const dir_gem  $(Convert-Path $PSScriptRoot\..)
Make-Const dir_ps   $PSScriptRoot

Push-Location $PSScriptRoot
.\shared\make.ps1
# .\shared\test.ps1
Pop-Location

exit $ttl_errors_fails + $exit_code
