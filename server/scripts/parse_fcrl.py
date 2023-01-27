# export $(cat .env | xargs)
import os, json
from datetime import datetime
import psycopg2
import psycopg2.extras
pg_conn = psycopg2.connect(
    dbname = os.environ.get('DB_NAME'),
    user = os.environ.get('DB_USER'),
    host = os.environ.get('DB_HOST', 'localhost'),
    password = os.environ.get('DB_PASSWORD')
)

SRC = '/mnt/cr55/FCRL_Data/Result/'

if __name__ == '__main__':
    with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../config.json')) as file:
        config = json.load(file)
    ch = int(input('\n'.join([f'{i}. {d}' for i, d in enumerate(config['devices'].keys())]) + '\ndevice: '))
    dev = list(config['devices'].keys())[ch]
    year = int(input('year: '))
    month = int(input('month: '))
    parse = ['Up', 'Lo', 'CC', 'Pr']
    columns = config['devices'][dev]['counters'] + ['pressure']
    print('\n' + '\n'.join([f'{l} -> {r}' for l, r in zip(parse, columns)]))

    data = []
    src = (SRC + 'Result_Pre_Final/') if datetime.now().year % 100 == year and datetime.now().month == month else SRC
    print(src, year, month)
    fn = f'{src}{(year%100):02d}{month:02d}FCRL_Result.01u.txt'
    print('reading', fn)
    with open(fn) as f:
        lines = f.readlines()
    indexes = [lines[0].split().index(c) for c in columns]
    for line in lines[2:]:
        sp = line.split()
        time = sp[0]+' '+sp[1]
        data.append([time] + [sp[i] for i in indexes])
    print(f'\ninserting {len(data)} rows')
    with pg_conn.cursor() as cursor:
        q = f'INSERT INTO {dev}_raw (time, {", ".join(columns)}) VALUES %s ON CONFLICT(time) DO NOTHING'
        psycopg2.extras.execute_values(cursor, q, data)
    if input('done! commit? [y/n]: ').lower() == 'y':
        pg_conn.commit()


