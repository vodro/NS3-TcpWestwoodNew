from asyncio import protocols
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd


prot = ['TcpNewReno', 'TcpWestwood', 'TcpWestWoodNew']


def plot(dfs):

    col_names = df.keys().values.tolist()

    for i in range(1, len(col_names)):
        time_col = df[col_names[0]].tolist()
        val_col = df[col_names[i]].tolist()
        print(val_col)
        plt.plot(time_col, val_col, label='TcpWestWoodNew')
        plt.title(col_names[i] + ' vs ' + col_names[0])
        plt.xlabel(col_names[0])
        plt.ylabel(col_names[i])
        plt.grid()
        plt.show()


if __name__ == '__main__':
    df = pd.read_csv('../_temp/now/TcpWestwood_state.csv')
    plot(df)
