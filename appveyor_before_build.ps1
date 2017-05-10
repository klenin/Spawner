$build_dir = $env:build_dir
if (Test-Path -Path $build_dir) {
    Remove-Item -Path $build_dir -Recurse -Force
}
$hide_output = New-Item -Path $build_dir -ItemType "directory"
Set-Location $build_dir
cmake ..
Set-Location ..
