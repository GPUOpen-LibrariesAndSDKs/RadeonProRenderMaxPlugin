Param(
	[Parameter(Mandatory=$True)]
	[string]$cfg
)

$targetPath = "$Env:LOCALAPPDATA\Autodesk\3dsMax\$cfg - 64bit\ENU\Plugin.UserSettings.ini"

if (-not (Test-Path $targetPath)) {
	return
}

$rootPath = Split-Path $MyInvocation.MyCommand.Path -Parent
$rootPath = Split-Path $rootPath -Parent

$pluginPath = "$rootPath\dist\plug-ins\$cfg"

$content= Get-Content -Path $targetPath -Encoding Unicode

$nl = [Environment]::NewLine

foreach ($line in $content) {
	$tline = $line.trim()

	if ($tline -eq "[Directories]") {
		$out += $tline + $nl
		$out += "AMD Radeon ProRender=$pluginPath$nl"
	} elseif ($tline -match "AMD Radeon ProRender") {
		#don't post the line to output
	}
	elseif ( -not [string]::IsNullOrEmpty($tline) ) {
		$out += $tline + $nl
	}
}

Set-Content -Path $targetPath -Encoding Unicode $out
