import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import sys


params = ['node', 'flow']

task_name = 'a1'


def plot(df):

    col_names = df.keys().values.tolist()
    print(df.head(0))

    for i in range(1, len(col_names)):
        time_col = df[col_names[0]].tolist()
        val_col = df[col_names[i]].tolist()
        # print(val_col)
        plt.plot(time_col, val_col, label=df.name)
        plt.legend()
        plt.title(col_names[i] + ' vs ' + col_names[0])
        plt.xlabel(col_names[0])
        plt.ylabel(col_names[i])
        plt.grid()

        plt.savefig('./plots/' + task_name + '/' +
                    col_names[i].strip() + '_vs_' + col_names[0] + '.pdf')
        plt.clf()
        # plt.show()


if __name__ == '__main__':
    task_name = sys.argv[1]
    print(task_name)
    dfs = []
    for param in params:
        print('../_temp/' + task_name +
              '/' + param + '_details.csv')
        df = pd.read_csv('../_temp/' + task_name +
                         '/' + param + '_details.csv')
        df.name = param
        plot(df)
