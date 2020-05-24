#!/usr/bin/python3
import pandas as pd
import plotly.express as px
import plotly.io as pio
import plotly.graph_objects as go

data = pd.read_csv("./test.log", sep=";")
data['Time'] = pd.to_datetime(data['Time'], unit='ms', origin='unix', utc=True)

fig = go.Figure()
fig.add_trace(go.Scatter(x=data["Time"], y=data["Value1"],
                    mode='lines+markers',
                    name='Sensor 1'))
fig.add_trace(go.Scatter(x=data["Time"], y=data["Value2"],
                    mode='lines+markers',
                    name='Sensor 2'))

fig.update_layout(xaxis = dict(tickformat = "%Y-%m-%d %H:%M:%S.%L"))
fig.update_layout(title='Wifi Temperature Sensor',
                   xaxis_title='Time',
                   yaxis_title='Temperature (degree Celsius)')

pio.write_html(fig, file="plot.html", auto_open=False)
