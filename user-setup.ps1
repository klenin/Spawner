param(
    [String]$username='runner',
    [String]$password='12345',
    [String]$dir='.',
    [String[]]$canRead=@(),
    [String[]]$canWrite=@()
)

function AddAccessRules($UserName, [String[]]$Rights, [String]$Path, [String]$Type) {
    $user = New-Object System.Security.Principal.NTAccount($UserName)
    
    New-Item -Path $Path -ItemType $Type -Force
    
    $colRights = [System.Security.AccessControl.FileSystemRights]($Rights -join ', ')
    
    $accessRule = New-Object System.Security.AccessControl.FileSystemAccessRule `
    (
        $user,
        $colRights,
        [System.Security.AccessControl.InheritanceFlags]::None,
        [System.Security.AccessControl.PropagationFlags]::None,
        [System.Security.AccessControl.AccessControlType]::Allow
    )
    
    $acl = Get-ACL -path $path
    
    $acl.AddAccessRule($accessRule)
    
    Set-ACL -Path $path -AclObject $acl
}

$Computer = [ADSI]"WinNT://$Env:COMPUTERNAME,Computer"

try
{
    $computer.delete('user', $username);
}
catch
{}

$runner = $Computer.Create("User", $username)
$runner.SetPassword($password)
$runner.SetInfo()
$runner.UserFlags = 64 + 65536 # ADS_UF_PASSWD_CANT_CHANGE + ADS_UF_DONT_EXPIRE_PASSWD
$runner.SetInfo()

AddAccessRules -UserName $username -Rights 'Read' -Path $dir -Type 'dir'

foreach ($file in $canRead) {
     AddAccessRules -UserName $username -Rights 'Read' -Path ($dir + '\' + $file) -Type 'file'
}

foreach ($file in $canWrite) {
    AddAccessRules -UserName $username -Rights 'Write' -Path ($dir + '\' + $file) -Type 'file'
}
