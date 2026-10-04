param(
    [string]$SourceDir = (Join-Path $PSScriptRoot '..\extras\livearea\source')
)
$ErrorActionPreference = 'Stop'
$source = (Resolve-Path -LiteralPath $SourceDir).Path
$output = Join-Path $PSScriptRoot '..\extras\livearea'
$preview = Join-Path $PSScriptRoot '..\analysis\livearea'
New-Item -ItemType Directory -Force -Path $output, $preview | Out-Null

function Invoke-Magick([string[]]$Arguments) {
    & magick @Arguments
    if ($LASTEXITCODE -ne 0) { throw 'LiveArea image conversion failed.' }
}

# Mechanical size/encoding conversions only. The workshop masters come from
# imagegen; the icon and launch tile retain the supplied artwork.
$palette = @('-strip', '-colorspace', 'sRGB', '-alpha', 'off', '+dither', '-colors', '256', '-depth', '8')
Invoke-Magick (@((Join-Path $source 'icon.webp'), '-background', '#21699e', '-alpha', 'remove',
    '-filter', 'Lanczos', '-resize', '128x128!') + $palette + @('PNG8:' + (Join-Path $output 'icon0.png')))
Invoke-Magick (@((Join-Path $source 'livearea.png'), '-filter', 'Lanczos', '-resize', '840x500!') +
    $palette + @('PNG8:' + (Join-Path $output 'bg0.png')))
# Preserve the entire character and logo when fitting the wider launch image.
Invoke-Magick (@((Join-Path $source 'splash.png'), '-filter', 'Lanczos', '-resize', '960x544!') +
    $palette + @('PNG8:' + (Join-Path $output 'pic0.png')))
Invoke-Magick (@((Join-Path $source 'launch.jpg'), '-filter', 'Lanczos', '-resize', '280x158!') +
    $palette + @('PNG8:' + (Join-Path $output 'startup.png')))

# Layout preview only. The actual shell draws its own border and Start button.
# Style a1 places the 280x158 gate at (280,139) within the 840x500 background.
Invoke-Magick @((Join-Path $output 'bg0.png'), (Join-Path $output 'startup.png'),
    '-gravity', 'northwest', '-geometry', '+280+139', '-compose', 'over', '-composite',
    '-fill', 'none', '-stroke', '#edf3ed', '-strokewidth', '3', '-draw', 'roundrectangle 280,139 560,297 8,8',
    '-stroke', 'none', '-fill', '#14a8de', '-draw', 'roundrectangle 360,257 480,289 10,10',
    '-font', 'Arial-Bold', '-pointsize', '16', '-fill', 'white', '-gravity', 'northwest', '-annotate', '+400+261', 'Start',
    (Join-Path $preview 'livearea-preview.png'))
Write-Output "Prepared LiveArea assets in $output"
Write-Output "Layout preview: $preview\livearea-preview.png"
