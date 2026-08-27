param(
    [string]$HubName = "guardiantone-iot",
    [string]$ConsumerGroup = "guardiantone-dashboard",
    [string]$DeviceId = "guardiantone-esp32"
)

$ErrorActionPreference = "Stop"

Write-Host "Creating/confirming consumer group '$ConsumerGroup'..."
az iot hub consumer-group create --hub-name $HubName --name $ConsumerGroup --output none 2>$null

$endpoint = az iot hub show --name $HubName --query "properties.eventHubEndpoints.events.endpoint" -o tsv
$entityPath = az iot hub show --name $HubName --query "properties.eventHubEndpoints.events.path" -o tsv
$serviceKey = az iot hub policy show --hub-name $HubName --name service --query "primaryKey" -o tsv

if (-not $endpoint -or -not $entityPath -or -not $serviceKey) {
    throw "Could not read the IoT Hub Event Hub-compatible endpoint or service key. Make sure 'az login' works and you have access to the hub."
}

$connectionString = "Endpoint=$endpoint;SharedAccessKeyName=service;SharedAccessKey=$serviceKey;EntityPath=$entityPath"
$envPath = Join-Path $PSScriptRoot ".env"

@"
EVENTHUB_CONNECTION_STRING=$connectionString
EVENTHUB_CONSUMER_GROUP=$ConsumerGroup
DEVICE_ID=$DeviceId
EVENTHUB_NAME=
"@ | Set-Content -Path $envPath -Encoding utf8

Write-Host "Created $envPath"
Write-Host "The file contains a service key and is ignored by Git. Do not share or commit it."
