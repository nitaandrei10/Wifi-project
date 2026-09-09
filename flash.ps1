# Parameters with default values
param (
    [string]$Port = "COM5",
    [string]$Baud = "921600",
    [string]$Binary = "build/app.bin"
)

# Check if the compiled binary actually exists before attempting to flash
if (-not (Test-Path $Binary)) {
    Write-Error "Binary file '$Binary' not found! Did you run 'idf.py build' inside Docker first?"
    exit 1
}

Write-Host "Flashing $Binary to $Port at $Baud baud..." -ForegroundColor Cyan

# Execute esptool
python -m esptool --port $Port --baud $Baud --chip esp32 --before default_reset --after hard_reset write_flash --flash_freq 40m --flash_size detect 0x10000 $Binary

if ($LASTEXITCODE -eq 0) {
    Write-Host "Flash successful!" -ForegroundColor Green
} else {
    Write-Host "Flashing failed. (If stuck on 'Connecting...', hold the BOOT button on the ESP32)." -ForegroundColor Red
}