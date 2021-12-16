$profile_filename = "memory_profile.json"

Remove-Item $profile_filename

Add-Content $profile_filename "["

for ($i = 0; $i -lt (60 * 60); $i++) {
    Write-Output "$i"
    $memory_json = azsphere device app show-memory-stats --output json
    $memory_json = $memory_json + ","
    Add-Content $profile_filename $memory_json
    # Start-Sleep -Seconds 1
}

Add-Content $profile_filename "]"