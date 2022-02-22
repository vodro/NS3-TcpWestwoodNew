from asyncio import protocols
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd


prots = ['TcpNewReno', 'TcpWestwood', 'TcpWestwoodNew']


def plot(dfs):

    col_names = dfs[0].keys().values.tolist()
    print(dfs[0].head(0))

    for i in range(1, len(col_names)):

        for df in dfs:
            time_col = df[col_names[0]].tolist()
            val_col = df[col_names[i]].tolist()
            # print(val_col)
            plt.plot(time_col, val_col, label=df.name)
        plt.legend()
        plt.title(col_names[i] + ' vs ' + col_names[0])
        plt.xlabel(col_names[0])
        plt.ylabel(col_names[i])
        plt.grid()
        plt.show()


if __name__ == '__main__':
    dfs = []
    for prot in prots:
        df = pd.read_csv('../_temp/paper/' + prot + '_state.csv')
        df.name = prot
        dfs.append(df)
    plot(dfs)
