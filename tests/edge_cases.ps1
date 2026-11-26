# Build the C++ and Rust executables, then run:
# .\tests\edge_cases.ps1 -CppExe <cpp-executable> -RustExe <rust-executable>
param(
    [Parameter(Mandatory = $true)][string]$CppExe,
    [Parameter(Mandatory = $true)][string]$RustExe
)

$ErrorActionPreference = 'Stop'

$cases = @(
    @{
        Name = 'missing names leave selection unchanged'
        Input = @'
info
move missing 1 1
worldpos missing
intersects root missing
intersects missing root
swap missing root
erase missing
select missing
add missing child 0 0 1 1
paste missing
delete-selected
cut
select
add root present 1 2 3 4
select present
select missing
info
quit
'@
        Expected = @'
NO_SELECTION
ERR
ERR
ERR
ERR
ERR
ERR
ERR
ERR
ERR
ERR
ERR
ERR usage (type help)
OK
OK
ERR
SELECTED present
DROP root
DROP present
'@
    },
    @{
        Name = 'duplicate names in scene and clipboard and selection after paste'
        Input = @'
add root group 10 20 10 10
add group item 1 2 3 4
add root item 0 0 1 1
add root root 0 0 1 1
select group
cut
info
select item
add root group 0 0 1 1
add root item 0 0 1 1
paste missing
info
add root destination 5 6 10 10
paste destination
info
select item
info
worldpos item
add root item 0 0 1 1
paste root
delete-selected
info
quit
'@
        Expected = @'
OK
OK
ERR
ERR
OK
OK
SELECTED group
ERR
ERR
ERR
ERR
SELECTED group
OK
OK
SELECTED group
OK
SELECTED item
WORLD 16 28
ERR
ERR
AFTER_DETACH item
DROP item
OK
EXPIRED
DROP root
DROP destination
DROP group
'@
    },
    @{
        Name = 'self swap and root swap do not mutate positions'
        Input = @'
add root a 1 2 3 4
add root b 5 6 7 8
swap a a
swap root root
swap a root
swap a missing
worldpos a
worldpos b
intersects a a
swap a b
worldpos a
worldpos b
quit
'@
        Expected = @'
OK
OK
ERR
ERR
ERR
ERR
WORLD 1 2
WORLD 5 6
YES
OK
WORLD 5 6
WORLD 1 2
DROP root
DROP a
DROP b
'@
    },
    @{
        Name = 'deleting selected descendant group expires selection'
        Input = @'
add root group 0 0 10 10
add group child 1 2 3 4
select child
erase group
info
delete-selected
cut
add root child 7 8 3 4
info
select child
info
worldpos child
quit
'@
        Expected = @'
OK
OK
OK
DROP group
DROP child
OK
EXPIRED
ERR
ERR
OK
EXPIRED
OK
SELECTED child
WORLD 7 8
DROP root
DROP child
'@
    }
)

foreach ($implementation in @(
    @{ Name = 'C++'; Exe = $CppExe },
    @{ Name = 'Rust'; Exe = $RustExe }
)) {
    foreach ($case in $cases) {
        $actual = @($case.Input -split "`n" | & $implementation.Exe)
        if ($LASTEXITCODE -ne 0) {
            throw "$($implementation.Name): $($case.Name) exited with code $LASTEXITCODE"
        }
        $actualText = $actual -join "`n"
        $expectedText = ($case.Expected -split "`r?`n") -join "`n"
        if ($actualText -cne $expectedText) {
            throw "$($implementation.Name): $($case.Name) failed`nExpected:`n$expectedText`nActual:`n$actualText"
        }
        Write-Host "PASS $($implementation.Name): $($case.Name)"
    }
}
