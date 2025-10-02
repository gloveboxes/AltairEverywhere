# Create an Azure Streaming Analytics Job

From the Azure management portal

1. Login to the [Azure web portal](http://portal.azure.com/).
1. Select the ClimateMonitor resource group
1. Select the **weather-asa** Azure Stream Analytics Job.

## Create an input stream

1. Select **Inputs** from the Stream analytics job sidebar menu.
1. Select **+Add stream input**.
1. Select **Event hub**.
1. Name the input **weather-eh**.
1. Select **Select Event Hub from your subscription**.
1. Select existing hub namespace **weather-hub**.
1. Select **weather-hub** from existing Event hubs.
1. Create a new Event Hub consumer group.
1. Select **Connection string** authentication,
1. Create a new Event Hub policy.
1. Select **Save**.

## Create an anomaly-data output stream

1. Select **Outputs** from the Stream analytics job sidebar menu.
1. Select **+ Add**.
1. Select **Blog storage/ADLS Gen2**.
1. Name the output **anomaly-data**.
1. Select **Your subscription**.
1. Select the **Weatherstg** storage account you created.
1. Create a new container named **anomaly-data**.
1. Select **Connection string** authentication.
1. Set the path pattern to **{DeviceId}**.
1. Select **Save**

## Create an weather-log output stream

1. Select **Outputs** from the Stream analytics job sidebar menu.
1. Select **+ Add**.
1. Select **Blog storage/ADLS Gen2**.
1. Name the output **anomaly-data**.
1. Select **Your subscription**.
1. Select the **Weatherstg** storage account you created.
1. Create a new container named **weather-log**.
1. Select **Connection string** authentication.
1. Set the path pattern to **{DeviceId}**.
1. Select **Save**

## Define the query

1. Select **Query** from the Stream analytics job sidebar menu.
1. Paste in the following query

    ```sql
    -- https://docs.microsoft.com/en-us/azure/stream-analytics/stream-analytics-parsing-json
    
    WITH Telemetry AS (
        SELECT
            deviceId as DeviceId,
            enrichments.deviceName as DeviceName,
            telemetry.latitude as Latitude,
            telemetry.longitude as Longitude,
            MAX(telemetry.temperature) AS Temperature,
            AVG(telemetry.humidity) AS Humidity,
            AVG(telemetry.pressure) AS Pressure,
            MAX(enqueuedTime) AS TimeStamp,
            Count(*) as Count
        FROM [weather-eh] TIMESTAMP BY enqueuedTime
        GROUP BY
            deviceId,
            enrichments.deviceName,
            telemetry.latitude,
            telemetry.longitude,
            TumblingWindow(minute, 1)
    )
    
    SELECT DeviceId, Timestamp as timestamp, Temperature AS value INTO [anomaly-data] FROM Telemetry
    SELECT * INTO [weather-log] FROM Telemetry
    ```

1. Select **Save**

## Set Streaming units

Minimize the cost of Stream Analytics and set the streaming units to 1.

1. Select **Scale**.
1. Set **Streaming units** to 1.
1. Select **Save**.

## Start the stream job

1. Select **Overview**.
1. Select **Start**.
1. Select **Start** to confirm start the job now.
