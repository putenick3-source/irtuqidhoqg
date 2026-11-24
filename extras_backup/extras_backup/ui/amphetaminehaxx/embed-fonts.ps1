$ErrorActionPreference = 'Stop'
$assetDirectory = Join-Path $PSScriptRoot 'fonts'
$target = Join-Path (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '../..')).ProviderPath 'build\otc_fonts.inc'
$names = [ordered]@{
    px='fs-tahoma-8px.otf'; flag='smallest_pixel-7.ttf'; tahoma='tahomabd.ttf'
    verdana_bold='verdanab.ttf'; verdana='verdana.ttf'
}
$writer = [IO.StreamWriter]::new($target, $false, [Text.Encoding]::ASCII)
try {
    foreach ($entry in $names.GetEnumerator()) {
        $bytes = [IO.File]::ReadAllBytes((Join-Path $assetDirectory $entry.Value))
        $writer.WriteLine("static unsigned char otc_$($entry.Key)[] = {")
        for ($offset=0; $offset -lt $bytes.Length; $offset+=32) {
            $size = [Math]::Min(32, $bytes.Length-$offset)
            $hex = [BitConverter]::ToString($bytes,$offset,$size)
            $writer.WriteLine('0x' + $hex.Replace('-',',0x') + ',')
        }
        $writer.WriteLine('};')
    }
} finally { $writer.Dispose() }

