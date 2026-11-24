$ErrorActionPreference = 'Stop'
$root     = Split-Path -Parent $MyInvocation.MyCommand.Path
$builds   = Join-Path $root 'builds'
$dll      = Join-Path $builds 'RobRunInternal.dll'
$mapper   = Join-Path $builds 'orelia-mapper.exe'
$driver   = Join-Path $builds 'orelia-ioctl.sys'
$injector = Join-Path $builds 'ioctl-injector.exe'

function Ok($m)   { Write-Host "  [ok]   $m" -ForegroundColor DarkGreen }
function Warn($m) { Write-Host "  [note] $m" -ForegroundColor Yellow }
function Die($m, $h) {
    Write-Host ""
    Write-Host "  [STOP] $m" -ForegroundColor Red
    if ($h) { Write-Host "         $h" -ForegroundColor Gray }
    Write-Host ""
    Read-Host "Entree pour fermer"
    exit 1
}

Clear-Host
Write-Host ""
Write-Host "  ROB RUN loader - verification" -ForegroundColor White
Write-Host ""

$admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $admin) { Die "Le script n'est pas en administrateur." "Clic droit sur start.bat puis Executer en tant qu'administrateur." }
Ok "administrateur"

foreach ($f in @($dll, $mapper, $driver, $injector)) {
    if (-not (Test-Path -LiteralPath $f)) {
        Die ("Fichier manquant : " + (Split-Path -Leaf $f)) "Re-dezippe le dossier en entier. L'antivirus a peut-etre supprime un fichier."
    }
}
Ok "fichiers du loader presents"

$build = 0
try { $build = [int](Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion').CurrentBuildNumber } catch {}
if ($build -lt 19044) { Die "Windows trop ancien (build $build)." "Minimum : Windows 10 22H2, build 19045." }
if ($build -lt 22621) { Warn "Windows 10 detecte (build $build) : support partiel, ca peut echouer." }
Ok "version de Windows (build $build)"

$hvci = -1
try { $hvci = [int](Get-ItemProperty 'HKLM:\SYSTEM\CurrentControlSet\Control\DeviceGuard\Scenarios\HypervisorEnforcedCodeIntegrity' -ErrorAction Stop).Enabled } catch {}
if ($hvci -eq 1) {
    Die "Memoire virtuelle securisee (HVCI) activee : le driver sera refuse." "Securite Windows > Securite de l'appareil > Isolement du noyau > Memoire virtuelle securisee : desactiver, puis REDEMARRER."
}
Ok "HVCI desactivee"

$cles = @('HKLM:\SYSTEM\CurrentControlSet\Control\CI\Policy','HKLM:\SYSTEM\CurrentControlSet\Control\CI\Config')
$blk = -1
foreach ($k in $cles) {
    try {
        $v = (Get-ItemProperty -LiteralPath $k -Name VulnerableDriverBlocklistEnable -ErrorAction Stop).VulnerableDriverBlocklistEnable
        if ($null -ne $v) { $blk = [int]$v }
    } catch {}
}
if ($blk -eq 1) {
    Warn "liste de blocage des pilotes vulnerables activee : le driver sera refuse au chargement"
    $r = Read-Host "         La desactiver maintenant ? (O/N, redemarrage requis)"
    if ($r -match '^[oOyY]') {
        foreach ($k in $cles) {
            try { Set-ItemProperty -LiteralPath $k -Name VulnerableDriverBlocklistEnable -Value 0 -Type DWord -ErrorAction Stop } catch {}
        }
        Ok "liste de blocage desactivee, redemarrage requis"
    } else { Warn "liste de blocage laissee activee" }
} elseif ($blk -eq 0) { Ok "liste de blocage des pilotes desactivee" }

try {
    $paths = @((Get-MpPreference -ErrorAction Stop).ExclusionPath)
    $excluded = $false
    foreach ($p in $paths) { if ($p -and $root.StartsWith($p, 'OrdinalIgnoreCase')) { $excluded = $true } }
    if ($excluded) { Ok "dossier exclu de l'antivirus" }
    else {
        Warn "ce dossier n'est pas exclu de l'antivirus"
        $r = Read-Host "         Ajouter l'exclusion maintenant ? (O/N)"
        if ($r -match '^[oOyY]') { Add-MpPreference -ExclusionPath $root; Ok "exclusion ajoutee" }
        else { Warn "exclusion non ajoutee : l'antivirus peut supprimer le DLL" }
    }
} catch { Warn "Defender non interrogeable, on continue" }

$game = Get-Process -Name 'ROB RUN' -ErrorAction SilentlyContinue
if (-not $game) { Die "ROB RUN n'est pas lance." "Lance le jeu, connecte-toi (menu ou partie), puis relance start.bat." }
Ok ("jeu en cours, PID " + $game.Id)

$hook = $false
try { $hook = [bool]($game.Modules | Where-Object { $_.ModuleName -eq 'graphics-hook64.dll' }) } catch {}
if ($hook) { Ok "hook de capture OBS en place" }
else { Warn "hook OBS non detecte (non bloquant) : si le menu ne s'ouvre pas, verifie OBS en administrateur et la capture de jeu" }

Write-Host ""
$stale = @()
try {
    $stale = Get-ChildItem 'HKLM:\SYSTEM\CurrentControlSet\Services' -ErrorAction Stop | Where-Object {
        $ip = (Get-ItemProperty -LiteralPath $_.PSPath -ErrorAction SilentlyContinue).ImagePath
        $ip -and ($ip -match 'orelia')
    }
} catch {}
if ($stale.Count -gt 0) {
    Warn ("nettoyage de " + $stale.Count + " service(s) residuel(s)")
    foreach ($s in $stale) {
        sc.exe stop $s.PSChildName | Out-Null
        sc.exe delete $s.PSChildName | Out-Null
    }
    Start-Sleep -Seconds 1
}

$mapOut = ''
$mapped = $false
for ($essai = 1; $essai -le 4; $essai++) {
    Write-Host ("  mapping du driver... (tentative " + $essai + "/4)") -ForegroundColor White
    $mapOut = (cmd /c "`"$mapper`" `"$driver`" <nul 2>&1") | Out-String
    if ($mapOut -match 'Successfully mapped module') { $mapped = $true; break }

    $nom = $null
    if ($mapOut -match 'Installing service:\s*(\S+)') { $nom = $Matches[1] }
    if ($nom) {
        $scOut = (sc.exe delete $nom 2>&1) | Out-String
        Start-Sleep -Seconds 2
        $encore = Test-Path ("HKLM:\SYSTEM\CurrentControlSet\Services\" + $nom)
        Write-Host ("         service " + $nom + " : suppression demandee, encore present = " + $encore) -ForegroundColor Yellow
        if ($scOut -match 'FAILED|refus') { Write-Host ("         " + $scOut.Trim()) -ForegroundColor DarkGray }
        continue
    }
    break
}
if (-not $mapped) {
    Write-Host $mapOut -ForegroundColor DarkGray
    if ($mapOut -match 'C5|allocat') { Die "Mapping refuse : etat du driver." "Redemarre le PC, puis refais toute la sequence." }
    Die "Mapping echoue apres 4 tentatives." "Redemarre le PC. Si un service reste present ci-dessus, seul un redemarrage le libere."
}
Ok "driver mappe"

Write-Host "  injection..." -ForegroundColor White
$injOut = (cmd /c "`"$injector`" `"$dll`" <nul 2>&1") | Out-String
if ($injOut -notmatch 'Successfully initialized module') {
    Write-Host $injOut -ForegroundColor DarkGray
    if ($injOut -match 'C5|allocat') { Die "Injection refusee : etat du driver." "Redemarre le PC et refais la sequence." }
    Die "Injection echouee." "Verifie que le jeu tourne encore et que l'apercu OBS est vivant. Puis redemarre le PC."
}
Ok "module injecte"

Write-Host ""
Write-Host "  [+] Pret. Retourne en jeu et appuie sur INSERT (HOME si rien)." -ForegroundColor Green
Write-Host ""
Start-Sleep -Seconds 6
