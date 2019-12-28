#!/usr/bin/python3
import pandas as pd
import plotly.express as px
import plotly.io as pio

data = pd.read_csv("./test.log", sep=";")

fig = px.line(data, x="Time", y="Value")

pio.write_html(fig, file="plot.html", auto_open=False)
