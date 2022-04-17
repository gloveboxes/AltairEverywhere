from azure.storage.blob import BlobServiceClient
from azure.storage.blob import ContainerClient
from sqlite3 import Timestamp
import json
import pandas as pd

input_container = "weather-tracker"
output_container = "learnoutputcontainer"
blob_conn_str = "DefaultEndpointsProtocol=https;AccountName=weatherstgglovebox;AccountKey=hTMFmZRiQVtpdDpEQNgEV3qtUNhVyKNmTWaA2sppDe6VPpy4bMnt1I9wwOwHk0Ysv2fqDg/HXUikpSd7/B2r1A==;EndpointSuffix=core.windows.net"
data = []
apikey = 'ANOMALY_DETECTION_API_KEY' 
endpoint = 'https://weather-anomaly.cognitiveservices.azure.com/anomalydetector/v1.0/timeseries/entire/detect'
deviceId = 'rpi44'

# Blob filter eg 00000000-7f2d-4ff9-0000-38749d4ebe61/23/2022/04/13/ to return just data from 2022/04/13
blob_filter = "00000000-7f2d-4ff9-0000-38749d4ebe61/23/2022/04/13/"


def format_json_data(telemetry_data, data):
    parse_json_records = [json.loads(str(item)) for item in telemetry_data.strip().split('\n')]
    for item in parse_json_records:
        if item["deviceId"] == deviceId:
            di = {}
            di['timestamp'] = item["enqueuedTime"]
            di['value'] = item['telemetry']['temperature']
            print(di)
            data.append(di)


def process_raw_data():
    container = ContainerClient.from_connection_string(
        conn_str=blob_conn_str, container_name=input_container)
    blob_name = ''
    json_paths = []
    blob_list = container.list_blobs(name_starts_with = blob_filter )
    for blob in blob_list:
        # read raw data from blob storage
        blob_name = blob.name
        # print(blob.name + '\n')
        blob_client = container.get_blob_client(blob_name)
        filestream = blob_client.download_blob()
        filecontents = filestream.content_as_text()

        if filecontents:
            format_json_data(filecontents, data)


process_raw_data()

# json.dump(data, open("data.txt",'w'))

# data = json.load(open("data.txt"))

df = pd.DataFrame(data)
df.index = pd.to_datetime(df['timestamp'])
res = df.resample('30min').max()
res = res.drop(columns=['timestamp'])
res = res.reset_index()

print(res)

result = res.to_json(date_format="iso", orient="records")

print(result)