# Build script for Charity Fund on Windows (MinGW-w64 toolchain via MSYS2, Qt6 + QPSQL)
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

$msys2Candidates = @(
    "$env:USERPROFILE\msys64",
    "C:\msys64",
    "C:\tools\msys64"
)
$msys2Root = $msys2Candidates | Where-Object { Test-Path "$_\mingw64\bin\g++.exe" } | Select-Object -First 1

if (-not $msys2Root) {
    Write-Error @"
MinGW-w64 Qt6 toolchain not found.
Install MSYS2 (https://www.msys2.org/) and run inside its shell:
  pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-charts mingw-w64-x86_64-postgresql
"@
    exit 1
}

$env:PATH = "$msys2Root\mingw64\bin;$env:PATH"
Write-Output "Toolchain: $msys2Root\mingw64\bin"

Write-Output "Configuring..."
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Output "Building..."
cmake --build build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Output "Deploying Qt DLLs next to the executable..."
# windeployqt6 routinely writes non-fatal warnings (e.g. missing translation
# catalogs) to stderr; under $ErrorActionPreference = "Stop" those get
# promoted to terminating errors and abort the script before all DLLs/plugins
# are copied. Run it with errors treated as non-terminating instead.
$previousErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = "Continue"
& "$msys2Root\mingw64\bin\windeployqt6.exe" "build\charity_fund.exe" 2>&1 | ForEach-Object { Write-Output $_ }
$ErrorActionPreference = $previousErrorActionPreference

# The app only ever uses the PostgreSQL (QPSQL) driver, but windeployqt6
# bundles every SQL driver plugin Qt was built with by default. Drop the
# ones we don't use — smaller install, and no SQLite/MySQL/ODBC/IBase
# drivers (and their runtime DLLs) shipped for no reason.
$sqlDriversDir = "$PSScriptRoot\build\sqldrivers"
if (Test-Path $sqlDriversDir) {
    Get-ChildItem -Path $sqlDriversDir -Filter "*.dll" |
        Where-Object { $_.Name -ne "qsqlpsql.dll" } |
        ForEach-Object {
            Write-Output "Removing unused SQL driver: $($_.Name)"
            Remove-Item $_.FullName -Force
        }
}
# Only qsqlite.dll pulls in its own extra runtime DLL (libsqlite3-0.dll);
# drop it too if a previous incremental build left it behind.
$orphanSqliteRuntime = "$PSScriptRoot\build\libsqlite3-0.dll"
if (Test-Path $orphanSqliteRuntime) { Remove-Item $orphanSqliteRuntime -Force }

# windeployqt6 is inconsistent about bundling the MinGW C/C++ runtime DLLs
# (libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll, etc.) — whether
# it treats them as "local" (needs copying) or "system" (skip) can vary run
# to run. Without them the exe fails to even start ("... .dll was not found").
# Walk the real dependency closure with objdump and copy anything still
# missing, so the exe never depends on MSYS2 being on PATH at runtime.
Write-Output "Verifying MinGW runtime DLL closure..."
$objdump = "$msys2Root\mingw64\bin\objdump.exe"
$mingwBin = "$msys2Root\mingw64\bin"
$buildDir = "$PSScriptRoot\build"
$queue = [System.Collections.Generic.Queue[string]]::new()
# Seed with the exe AND every DLL/plugin windeployqt6 already placed (including
# subfolders like sqldrivers/, imageformats/, platforms/) so the walk also
# picks up second-order MinGW deps such as libpq.dll (needed by qsqlpsql.dll),
# libsqlite3-0.dll (qsqlite.dll), libjpeg-8.dll (qjpeg.dll), etc. — not just
# the exe's own direct imports.
$queue.Enqueue("$buildDir\charity_fund.exe")
Get-ChildItem -Path $buildDir -Filter "*.dll" -Recurse | ForEach-Object { $queue.Enqueue($_.FullName) }
$visited = [System.Collections.Generic.HashSet[string]]::new()
$copied = @()

while ($queue.Count -gt 0) {
    $current = $queue.Dequeue()
    if (-not (Test-Path $current)) { continue }
    $importLines = & $objdump -p $current 2>$null | Select-String -Pattern '^\s*DLL Name:\s*(.+)$'
    foreach ($line in $importLines) {
        $dep = $line.Matches[0].Groups[1].Value.Trim()
        $depKey = $dep.ToLowerInvariant()
        if ($visited.Contains($depKey)) { continue }
        $visited.Add($depKey) | Out-Null
        $sourcePath = Join-Path $mingwBin $dep
        if (Test-Path $sourcePath) {
            $destPath = Join-Path $buildDir $dep
            if (-not (Test-Path $destPath)) {
                Copy-Item $sourcePath $destPath
                $copied += $dep
            }
            $queue.Enqueue($destPath)
        }
    }
}

if ($copied.Count -gt 0) {
    Write-Output "Copied missing runtime DLLs: $($copied -join ', ')"
} else {
    Write-Output "All runtime DLLs already present."
}

Write-Output ""
Write-Output "Done: build\charity_fund.exe"
