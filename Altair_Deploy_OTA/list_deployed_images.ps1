./config.ps1


Write-Host "`nListing images for device group id: $device_group from tenant id: $tenant_id`n"

azsphere device-group deployment list --device-group $device_group -t $tenant_id -o json
