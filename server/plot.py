#!/usr/bin/python3
import pandas as pd
import plotly.express as px
import plotly.io as pio

data = pd.read_csv("./test.log", sep=";")
data['Time'] = pd.to_datetime(data['Time'], unit='ms', origin='unix', utc=True)
fig = px.line(data, x="Time", y="Value")
fig.update_layout(xaxis = dict(tickformat = "%Y-%m-%d %H:%M:%S.%L"))

pio.write_html(fig, file="plot.html", auto_open=False)
