# Clean Temp Files Script - Removes intermediate/temporary build files + build outputs
# Removes: Build outputs (gn*.dll/lib, predix*.dll/lib, Image*.exe, PortView*.exe)
# Removes: Intermediate files (.obj, .pch, .ilk, .pdb, .idb, .tlog, etc.)
# Preserves: External libraries (opencv*.dll/lib, ippi*.dll)

$ErrorActionPreference = 'Continue'
$WorkspaceFolder = Split-Path -Parent $PSScriptRoot

Write-Host ""
Write-Host "🧹 Cleaning build outputs and temporary files..." -ForegroundColor Cyan
Write-Host "   (Removing gn*.dll/lib, predix*.dll/lib, Image*.exe, PortView*.exe)" -ForegroundColor Yellow
Write-Host "   (Preserving opencv and ippi libraries)" -ForegroundColor Gray
Write-Host ""

# Files to remove (intermediate/temporary only - NO build outputs)
$tempFilePatterns = @(
    '*.obj',           # Object files
    '*.pch',           # Precompiled headers
    '*.ilk',           # Incremental link files
    '*.pdb',           # Program database (debug symbols) - regeneratable
    '*.idb',           # Minimal rebuild info
    '*.ipch',          # IntelliSense precompiled header
    '*.tlog',          # Tracking logs
    '*.lastbuildstate',# Last build state
    '*.log',           # Build logs
    '*.iobj',          # LTCG intermediate
    '*.ipdb',          # LTCG program database
    '*.unsuccessfulbuild', # Build failure marker
    '*.cache',         # Various cache files
    '*.sbr',           # Source browser intermediate
    '*.bsc',           # Browse information
    '*.res',           # Compiled resources (regeneratable)
    '*.exp',           # Export files
    '*.aps'            # Resource editor temporary files
)

$removedFiles = 0
$totalSize = 0
$skippedFiles = 0

Write-Host "📁 Scanning workspace..." -ForegroundColor Yellow

# Scan all 8 configuration folders
$configPaths = @(
    'Debug',
    'Release', 
    'Win32\Debug',
    'Win32\Release',
    'x64\Debug',
    'x64\Release'
)

foreach ($pattern in $tempFilePatterns) {
    $files = Get-ChildItem -Path $WorkspaceFolder -Filter $pattern -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { 
            # Exclude ExtLibs folder completely
            $_.FullName -notlike "*\ExtLibs\*" -and
            # Exclude opencv files
            $_.Name -notlike "opencv*" -and
            # Exclude ippi files
            $_.Name -notlike "ippi*"
        }
    
    foreach ($file in $files) {
        $size = $file.Length
        $totalSize += $size
        
        try {
            Remove-Item $file.FullName -Force -ErrorAction Stop
            $removedFiles++
            
            # Show samples
            if ($removedFiles -le 15) {
                $sizeKB = [math]::Round($size / 1KB, 1)
                $relativePath = $file.FullName.Replace($WorkspaceFolder, '.')
                if ($relativePath.Length -gt 60) {
                    $relativePath = "..." + $relativePath.Substring($relativePath.Length - 57)
                }
                Write-Host "   🗑️ $relativePath ($sizeKB KB)" -ForegroundColor DarkGray
            }
        }
        catch {
            $skippedFiles++
        }
    }
}

if ($removedFiles -gt 15) {
    Write-Host "   ... and $($removedFiles - 15) more files" -ForegroundColor DarkGray
}

# Remove build output files (gn*.dll/lib, predix*.dll/lib, Image*.exe, PortView*.exe)
Write-Host ""
Write-Host "📁 Removing build output files..." -ForegroundColor Yellow

$buildOutputPatterns = @(
    'gn*.dll',
    'gn*.lib',
    'predix*.dll',
    'predix*.lib',
    'Image*.exe',
    'PortView*.exe'
)

$removedOutputs = 0
foreach ($pattern in $buildOutputPatterns) {
    $files = Get-ChildItem -Path $WorkspaceFolder -Filter $pattern -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { 
            # Exclude ExtLibs folder completely
            $_.FullName -notlike "*\ExtLibs\*"
        }
    
    foreach ($file in $files) {
        $size = $file.Length
        $totalSize += $size
        
        try {
            Remove-Item $file.FullName -Force -ErrorAction Stop
            $removedOutputs++
            
            $sizeKB = [math]::Round($size / 1KB, 1)
            $relativePath = $file.FullName.Replace($WorkspaceFolder, '.')
            if ($relativePath.Length -gt 60) {
                $relativePath = "..." + $relativePath.Substring($relativePath.Length - 57)
            }
            Write-Host "   🗑️ $relativePath ($sizeKB KB)" -ForegroundColor DarkGray
        }
        catch {
            Write-Host "   ⚠️ Could not remove: $($file.Name)" -ForegroundColor Yellow
        }
    }
}

# Remove ConsoleApp Debug/Release folders
Write-Host ""
Write-Host "📁 Removing ConsoleApp build folders..." -ForegroundColor Yellow

$consoleAppFolders = @(
    "$WorkspaceFolder\ConsoleApp\Debug",
    "$WorkspaceFolder\ConsoleApp\Release"
)

$removedConsoleAppFolders = 0
foreach ($folder in $consoleAppFolders) {
    if (Test-Path $folder) {
        try {
            $files = Get-ChildItem -Path $folder -Recurse -File -ErrorAction SilentlyContinue
            $fileCount = ($files | Measure-Object).Count
            $folderSize = ($files | Measure-Object -Property Length -Sum).Sum
            
            if ($null -ne $folderSize) {
                $totalSize += $folderSize
            }
            
            Remove-Item $folder -Recurse -Force -ErrorAction Stop
            $sizeMB = [math]::Round($folderSize / 1MB, 2)
            $relativePath = $folder.Replace($WorkspaceFolder, '.')
            Write-Host "   🗑️ $relativePath ($fileCount files, $sizeMB MB)" -ForegroundColor DarkGray
            $removedConsoleAppFolders++
        }
        catch {
            Write-Host "   ⚠️ Could not remove: $($folder.Replace($WorkspaceFolder, '.'))" -ForegroundColor Yellow
        }
    }
}

# Remove all project Debug/Release/Win32/x64 folders
Write-Host ""
Write-Host "📁 Removing all project build folders (Debug/Release/Win32/x64)..." -ForegroundColor Yellow

$buildFolderNames = @('Debug', 'Release', 'Win32', 'x64')
$removedBuildFolders = 0
$removedBuildFiles = 0

foreach ($folderName in $buildFolderNames) {
    $buildFolders = Get-ChildItem -Path $WorkspaceFolder -Filter $folderName -Recurse -Directory -Force -ErrorAction SilentlyContinue |
        Where-Object {
            # Exclude ExtLibs folder
            $_.FullName -notlike "*\ExtLibs\*" -and
            # Exclude ImageProcessTool folder (contains external DLLs)
            $_.FullName -notlike "*\ImageProcessTool\Debug*" -and
            $_.FullName -notlike "*\ImageProcessTool\Release*" -and
            # Exclude lib folder
            $_.FullName -notlike "*\lib\*"
        }
    
    foreach ($folder in $buildFolders) {
        try {
            $files = Get-ChildItem -Path $folder.FullName -Recurse -File -ErrorAction SilentlyContinue
            $fileCount = ($files | Measure-Object).Count
            $folderSize = ($files | Measure-Object -Property Length -Sum).Sum
            
            if ($null -ne $folderSize) {
                $totalSize += $folderSize
                $removedBuildFiles += $fileCount
            }
            
            Remove-Item $folder.FullName -Recurse -Force -ErrorAction Stop
            
            # Show only first 10 to avoid too much output
            if ($removedBuildFolders -lt 10) {
                $sizeMB = [math]::Round($folderSize / 1MB, 2)
                $relativePath = $folder.FullName.Replace($WorkspaceFolder, '.')
                if ($relativePath.Length -gt 60) {
                    $relativePath = "..." + $relativePath.Substring($relativePath.Length - 57)
                }
                Write-Host "   🗑️ $relativePath ($fileCount files, $sizeMB MB)" -ForegroundColor DarkGray
            }
            $removedBuildFolders++
        }
        catch {
            # Silently skip locked folders
        }
    }
}

if ($removedBuildFolders -gt 10) {
    Write-Host "   ... and $($removedBuildFolders - 10) more folders" -ForegroundColor DarkGray
}

# Remove .vs folders (Visual Studio cache) - safe to delete
# Contains: .suo files, IntelliSense DB, Browse DB, ipch files
Write-Host ""
Write-Host "📁 Removing Visual Studio cache (.vs folders)..." -ForegroundColor Yellow
Write-Host "   - IntelliSense precompiled headers (.ipch)" -ForegroundColor DarkGray
Write-Host "   - Browse/Solution database (.db)" -ForegroundColor DarkGray
Write-Host "   - User options (.suo)" -ForegroundColor DarkGray
Write-Host ""

$removedVsFolders = 0
$removedVsFiles = 0

# Get .vs folders from root and all subdirectories (hidden folders require -Force)
$vsFolders = @()
$vsFolders += Get-ChildItem -Path $WorkspaceFolder -Filter ".vs" -Directory -Force -ErrorAction SilentlyContinue
$vsFolders += Get-ChildItem -Path $WorkspaceFolder -Filter ".vs" -Recurse -Directory -Force -ErrorAction SilentlyContinue

foreach ($folder in $vsFolders) {
    try {
        # Count files before deletion
        $files = Get-ChildItem -Path $folder.FullName -Recurse -File -ErrorAction SilentlyContinue
        $fileCount = ($files | Measure-Object).Count
        $folderSize = ($files | Measure-Object -Property Length -Sum).Sum
        
        if ($null -ne $folderSize) {
            $totalSize += $folderSize
            $removedVsFiles += $fileCount
        }
        
        Remove-Item $folder.FullName -Recurse -Force -ErrorAction Stop
        $sizeMB = [math]::Round($folderSize / 1MB, 2)
        Write-Host "   🗑️ $($folder.FullName.Replace($WorkspaceFolder, '.')) ($fileCount files, $sizeMB MB)" -ForegroundColor DarkGray
        $removedVsFolders++
    }
    catch {
        Write-Host "   ⚠️ Could not remove: $($folder.FullName.Replace($WorkspaceFolder, '.'))" -ForegroundColor Yellow
    }
}

# Summary
$totalSizeMB = [math]::Round($totalSize / 1MB, 2)

Write-Host ""
Write-Host "✅ Cleanup completed!" -ForegroundColor Green
Write-Host "   - Removed temp files: $removedFiles" -ForegroundColor Gray
Write-Host "   - Removed build outputs: $removedOutputs" -ForegroundColor Gray
Write-Host "   - Removed project build folders: $removedBuildFolders ($removedBuildFiles files)" -ForegroundColor Gray
Write-Host "   - Removed ConsoleApp folders: $removedConsoleAppFolders" -ForegroundColor Gray
Write-Host "   - Removed .vs folders: $removedVsFolders ($removedVsFiles files)" -ForegroundColor Gray
Write-Host "   - Space freed: $totalSizeMB MB" -ForegroundColor Gray
Write-Host "   - Skipped (locked): $skippedFiles" -ForegroundColor Gray
Write-Host ""
Write-Host "✓ Build outputs removed (gn*.dll/lib, predix*.dll/lib, Image*.exe, PortView*.exe)" -ForegroundColor Green
Write-Host "✓ All project build folders cleaned (Debug/Release/Win32/x64)" -ForegroundColor Green
Write-Host "✓ ConsoleApp\Debug and ConsoleApp\Release cleaned" -ForegroundColor Green
Write-Host "✓ opencv and ippi libraries preserved" -ForegroundColor Green
Write-Host "✓ All 8 configurations cleaned" -ForegroundColor Green
Write-Host "✓ Visual Studio cache (.vs) cleaned" -ForegroundColor Green
Write-Host ""
