./config.ps1

$image_id = "REPLACE_WITH_YOUR_IMAGE_ID"

azsphere image show -i $image_id -t $global:tenant_id