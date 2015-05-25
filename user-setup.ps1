param(
    [String]$username='runner',
    [String]$password='12345',
    [String]$dir='.',
    [String[]]$canRead=@(),
    [String[]]$canWrite=@()
)

function AddAccessRules($UserName, [String[]]$Rights, [String]$Path, [String]$Type, [System.Security.AccessControl.AccessControlType]$AccessControlType) {
    $user = New-Object System.Security.Principal.NTAccount($UserName)
    
    New-Item -Path $Path -ItemType $Type -Force
    
    $colRights = [System.Security.AccessControl.FileSystemRights]($Rights -join ', ')
    
    $accessRule = New-Object System.Security.AccessControl.FileSystemAccessRule `
    (
        $user,
        $colRights,
        [System.Security.AccessControl.InheritanceFlags]::None,
        [System.Security.AccessControl.PropagationFlags]::None,
        $AccessControlType
    )
    
    $acl = Get-ACL -path $path
    
    $acl.AddAccessRule($accessRule)
    
    Set-ACL -Path $path -AclObject $acl
}

function Allow($UserName, [String[]]$Rights, [String]$Path, [String]$Type) {
    AddAccessRules -username $UserName -rights $Rights -path $Path -type $Type -accessControlType ([System.Security.AccessControl.AccessControlType]::Allow)
}

function Deny($UserName, [String[]]$Rights, [String]$Path, [String]$Type) {
    AddAccessRules -username $UserName -rights $Rights -path $Path -type $Type -accessControlType ([System.Security.AccessControl.AccessControlType]::Deny)
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

Allow -UserName $username -Rights 'Read' -Path $dir -Type 'dir'
Deny -UserName $username -Rights 'Write', 'TakeOwnership', 'ChangePermissions', 'ReadPermissions' -Path $dir -Type 'dir'

foreach ($file in $canRead) {
     Allow -UserName $username -Rights 'Read' -Path ($dir + '\' + $file) -Type 'file'
     Deny -UserName $username -Rights 'Delete', 'Write', 'ChangePermissions', 'TakeOwnership' -Path ($dir + '\' + $file) -Type 'file'
}

foreach ($file in $canWrite) {
    Allow -UserName $username -Rights 'Write' -Path ($dir + '\' + $file) -Type 'file'
    Deny -UserName $username -Rights 'Delete', 'ChangePermissions', 'TakeOwnership' -Path ($dir + '\' + $file) -Type 'file'
}
