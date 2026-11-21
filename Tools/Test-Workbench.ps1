param(
 [string]$Workbench = 'C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger Tools/Workbench/ArmaReforgerWorkbenchSteamDiag.exe',
 [string]$BaseAddons = 'C:/Program Files (x86)/Steam/steamapps/common/Arma Reforger/addons',
 [string]$Profile = (Join-Path $env:TEMP ('BLCA_RUSH_Validation_' + [guid]::NewGuid().ToString('N')))
)
$ErrorActionPreference = 'Stop'
$blcaProjectRoot = Split-Path -Parent $PSScriptRoot
if (!(Test-Path -LiteralPath $Workbench)) { throw 'Supply the installed Workbench executable with -Workbench.' }
if (!(Test-Path -LiteralPath $BaseAddons)) { throw 'Supply vanilla addons with -BaseAddons.' }
$blcaArgs = @('-gproj', ('"' + (Join-Path $blcaProjectRoot 'addon.gproj') + '"'),
 '-addonsDir', ('"' + $BaseAddons + '"'), '-profile', ('"' + $Profile + '"'),
 '-wbModule=WorldEditor', '-plugin=BLCA_RushSurvey', '-wbsilent', '-maxFPS=15')
$blcaValidation = Start-Process -FilePath $Workbench -WorkingDirectory (Split-Path -Parent $Workbench) -ArgumentList $blcaArgs -WindowStyle Hidden -PassThru
Write-Output "Validation PID $($blcaValidation.Id). Logs: $Profile/logs"
