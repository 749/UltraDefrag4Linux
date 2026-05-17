param(
    [string]$BinaryPath = "src\bin\amd64\udefrag.exe",
    [string]$DriveLetter = "T",
    [int]$VolumeSizeMB = 1024,
    [int]$FillerFileCount = 300,
    [int]$FillerFileSizeMB = 3,
    [int]$FragmentedFileCount = 40,
    [int]$FragmentedFileSizeMB = 5,
    [string[]]$FileSystems = @(
        "FAT",
        "FAT32",
        "exFAT",
        "NTFS",
        "NTFS_COMPRESSED",
        "NTFS_MIXED",
        "UDF_102",
        "UDF_150",
        "UDF_200",
        "UDF_201",
        "UDF_250",
        "UDF_250_DUP"
    ),
    [switch]$FullFormat
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "==> $Message"
}

function Invoke-Checked {
    param(
        [string]$FilePath,
        [string[]]$ArgumentList
    )

    Write-Host "+ $FilePath $($ArgumentList -join ' ')"
    & $FilePath @ArgumentList
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath exited with code $LASTEXITCODE"
    }
}

function Invoke-DiskPart {
    param([string[]]$Commands)

    $scriptPath = Join-Path $env:TEMP ("udefrag-diskpart-{0}.txt" -f ([guid]::NewGuid()))
    try {
        $Commands | Set-Content -Path $scriptPath -Encoding ASCII
        Invoke-Checked -FilePath "diskpart.exe" -ArgumentList @("/s", $scriptPath)
    } finally {
        Remove-Item -Path $scriptPath -Force -ErrorAction SilentlyContinue
    }
}

function Test-Administrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Resolve-TestDriveLetter {
    param([string]$PreferredDriveLetter)

    $preferred = $PreferredDriveLetter.TrimEnd(":")
    if (-not (Test-Path "$preferred`:\")) {
        return $preferred
    }

    foreach ($candidate in @("T", "U", "V", "W", "X", "Y", "Z")) {
        if (-not (Test-Path "$candidate`:\")) {
            return $candidate
        }
    }

    throw "No free drive letter is available for the fragmented volume test."
}

function Get-TestFileSystem {
    param([string]$Name)

    switch ($Name.ToUpperInvariant()) {
        "FAT" {
            return [pscustomobject]@{ Name = "FAT"; FormatFs = "FAT"; UdfRevision = $null; UdfDuplicate = $false; Compression = "none" }
        }
        "FAT32" {
            return [pscustomobject]@{ Name = "FAT32"; FormatFs = "FAT32"; UdfRevision = $null; UdfDuplicate = $false; Compression = "none" }
        }
        "EXFAT" {
            return [pscustomobject]@{ Name = "exFAT"; FormatFs = "exFAT"; UdfRevision = $null; UdfDuplicate = $false; Compression = "none" }
        }
        "NTFS" {
            return [pscustomobject]@{ Name = "NTFS"; FormatFs = "NTFS"; UdfRevision = $null; UdfDuplicate = $false; Compression = "none" }
        }
        "NTFS_COMPRESSED" {
            return [pscustomobject]@{ Name = "NTFS compressed"; FormatFs = "NTFS"; UdfRevision = $null; UdfDuplicate = $false; Compression = "all" }
        }
        "NTFS_MIXED" {
            return [pscustomobject]@{ Name = "NTFS mixed"; FormatFs = "NTFS"; UdfRevision = $null; UdfDuplicate = $false; Compression = "mixed" }
        }
        "UDF_102" {
            return [pscustomobject]@{ Name = "UDF v1.02"; FormatFs = "UDF"; UdfRevision = "1.02"; UdfDuplicate = $false; Compression = "none" }
        }
        "UDF_150" {
            return [pscustomobject]@{ Name = "UDF v1.50"; FormatFs = "UDF"; UdfRevision = "1.50"; UdfDuplicate = $false; Compression = "none" }
        }
        "UDF_200" {
            return [pscustomobject]@{ Name = "UDF v2.00"; FormatFs = "UDF"; UdfRevision = "2.00"; UdfDuplicate = $false; Compression = "none" }
        }
        "UDF_201" {
            return [pscustomobject]@{ Name = "UDF v2.01"; FormatFs = "UDF"; UdfRevision = "2.01"; UdfDuplicate = $false; Compression = "none" }
        }
        "UDF_250" {
            return [pscustomobject]@{ Name = "UDF v2.50"; FormatFs = "UDF"; UdfRevision = "2.50"; UdfDuplicate = $false; Compression = "none" }
        }
        "UDF_250_DUP" {
            return [pscustomobject]@{ Name = "UDF v2.50 duplicated metadata"; FormatFs = "UDF"; UdfRevision = "2.50"; UdfDuplicate = $true; Compression = "none" }
        }
        default {
            throw "Unsupported filesystem test case: $Name"
        }
    }
}

function Format-TestVolume {
    param(
        [pscustomobject]$FileSystem,
        [string]$Drive
    )

    $formatMode = if ($FullFormat) { "" } else { "quick" }
    if ($FileSystem.FormatFs -eq "UDF") {
        $args = @($Drive, "/FS:UDF", "/V:UDFRAGCI", "/Y")
        if (-not $FullFormat) {
            $args += "/Q"
        }
        if ($FileSystem.UdfRevision) {
            $args += "/R:$($FileSystem.UdfRevision)"
        }
        if ($FileSystem.UdfDuplicate) {
            $args += "/D"
        }
        Invoke-Checked -FilePath "format.com" -ArgumentList $args
        return
    }

    Invoke-DiskPart @(
        "select volume $($Drive.TrimEnd(':'))",
        "format fs=$($FileSystem.FormatFs) label=UDFRAGCI $formatMode"
    )
}

function Get-ExtentCount {
    param([string]$Path)

    $output = & fsutil.exe file queryextents $Path 2>$null
    if ($LASTEXITCODE -ne 0) {
        return 0
    }

    return (($output | Select-String -Pattern "VCN:").Count)
}

function New-SizedFile {
    param(
        [string]$Path,
        [int]$SizeMB
    )

    Invoke-Checked -FilePath "fsutil.exe" -ArgumentList @(
        "file", "createnew", $Path, (($SizeMB * 1MB).ToString())
    )
}

function New-FragmentedDataset {
    param(
        [string]$Root,
        [string]$CompressionMode
    )

    $fillerRoot = Join-Path $Root "filler"
    $fragmentRoot = Join-Path $Root "fragmented"
    New-Item -ItemType Directory -Force -Path $fillerRoot, $fragmentRoot | Out-Null

    if ($CompressionMode -eq "all") {
        Invoke-Checked -FilePath "compact.exe" -ArgumentList @("/C", $fillerRoot, $fragmentRoot)
    }

    Write-Step "Creating allocation pressure files"
    for ($i = 0; $i -lt $FillerFileCount; $i++) {
        $file = Join-Path $fillerRoot ("filler-{0:D4}.bin" -f $i)
        New-SizedFile -Path $file -SizeMB $FillerFileSizeMB
    }

    Write-Step "Removing alternating files to create free-space holes"
    for ($i = 0; $i -lt $FillerFileCount; $i += 2) {
        Remove-Item -Force (Join-Path $fillerRoot ("filler-{0:D4}.bin" -f $i))
    }

    Write-Step "Creating test files expected to span multiple holes"
    for ($i = 0; $i -lt $FragmentedFileCount; $i++) {
        $file = Join-Path $fragmentRoot ("fragmented-{0:D4}.bin" -f $i)
        New-SizedFile -Path $file -SizeMB $FragmentedFileSizeMB
        if ($CompressionMode -eq "mixed" -and ($i % 4) -eq 0) {
            Invoke-Checked -FilePath "compact.exe" -ArgumentList @("/C", $file)
        }
    }

    if ($CompressionMode -eq "all") {
        Invoke-Checked -FilePath "compact.exe" -ArgumentList @("/C", "/S:$Root")
    }

    $fragmentedFiles = Get-ChildItem -Path $fragmentRoot -Filter "*.bin"
    $extentCounts = foreach ($file in $fragmentedFiles) {
        Get-ExtentCount -Path $file.FullName
    }

    $fragmentedCount = ($extentCounts | Where-Object { $_ -gt 1 }).Count
    $maxExtents = ($extentCounts | Measure-Object -Maximum).Maximum
    if ($null -eq $maxExtents) {
        $maxExtents = 0
    }

    Write-Host "Fragmented files detected: $fragmentedCount/$($fragmentedFiles.Count); max extents: $maxExtents"
    if ($fragmentedCount -eq 0) {
        Write-Warning "No fragmented files were detected. Continuing because allocation behavior can vary on virtual disks."
    }
}

function Invoke-FragmentedVolumeCase {
    param(
        [pscustomobject]$FileSystem,
        [string]$Binary,
        [string]$ResolvedDriveLetter
    )

    $drive = "$ResolvedDriveLetter`:"
    $safeName = ($FileSystem.Name -replace "[^A-Za-z0-9]+", "-").Trim("-").ToLowerInvariant()
    $vhdPath = Join-Path $env:TEMP "udefrag-fragmented-volume-$safeName.vhd"
    $testRoot = "$drive\udefrag-ci"

    try {
        Write-Step "Creating temporary ${VolumeSizeMB}MB VHD for $($FileSystem.Name) at $vhdPath"
        Remove-Item -Path $vhdPath -Force -ErrorAction SilentlyContinue
        Invoke-DiskPart @(
            "create vdisk file=`"$vhdPath`" maximum=$VolumeSizeMB type=fixed",
            "select vdisk file=`"$vhdPath`"",
            "attach vdisk",
            "create partition primary",
            "assign letter=$ResolvedDriveLetter"
        )

        Write-Step "Formatting test volume as $($FileSystem.Name)"
        Format-TestVolume -FileSystem $FileSystem -Drive $drive

        Write-Step "Running pre-test CHKDSK for $($FileSystem.Name)"
        Invoke-Checked -FilePath "chkdsk.exe" -ArgumentList @($drive)

        New-FragmentedDataset -Root $testRoot -CompressionMode $FileSystem.Compression

        Write-Step "Analyzing fragmented $($FileSystem.Name) test volume"
        Invoke-Checked -FilePath $Binary -ArgumentList @("--analyze", $drive)

        Write-Step "Running dry-run defragmentation against $($FileSystem.Name) test volume"
        $env:UD_DRY_RUN = "1"
        Invoke-Checked -FilePath $Binary -ArgumentList @("--defragment", $drive)

        Write-Step "Running post-test CHKDSK for $($FileSystem.Name)"
        Invoke-Checked -FilePath "chkdsk.exe" -ArgumentList @($drive)
    } finally {
        Remove-Item Env:\UD_DRY_RUN -ErrorAction SilentlyContinue

        Write-Step "Detaching and deleting temporary VHD for $($FileSystem.Name)"
        try {
            Invoke-DiskPart @(
                "select vdisk file=`"$vhdPath`"",
                "detach vdisk"
            )
        } catch {
            Write-Warning $_
        }
        Remove-Item -Path $vhdPath -Force -ErrorAction SilentlyContinue
    }
}

if (-not (Test-Administrator)) {
    throw "The fragmented volume test requires an elevated Windows runner."
}

$binary = Resolve-Path $BinaryPath
$resolvedDriveLetter = Resolve-TestDriveLetter -PreferredDriveLetter $DriveLetter
$fileSystemCases = foreach ($fs in $FileSystems) {
    Get-TestFileSystem -Name $fs
}

foreach ($fs in $fileSystemCases) {
    Write-Step "Starting filesystem case: $($fs.Name)"
    Invoke-FragmentedVolumeCase -FileSystem $fs -Binary $binary.Path -ResolvedDriveLetter $resolvedDriveLetter
}
