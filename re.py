import sys

# Читаем таблицу из файла
with open(sys.argv[1], 'r', encoding='utf-8') as f:
    lines = [line.strip() for line in f if line.strip() and not line.startswith('pfunc')]
    data = [line.split('\t') for line in lines]

# Спрашиваем что искать
query = input("Что ищем? (dll или функция): ")

if '.dll' in query.lower():  # ищем DLL (4-я колонка - sdllname)
    funcs = [row for row in data if len(row) >= 5 and row[3].lower() == query.lower()]
    with open(f'{query}_functions.txt', 'w', encoding='utf-8') as out:
        out.write('\t'.join(['pfunc', 'pbasedll', 'poffset', 'sdllname', 'sfuncname']) + '\n')
        for row in funcs:
            out.write('\t'.join(row) + '\n')
    print(f"Найдено {len(funcs)} функций, сохранено в {query}_functions.txt")
else:  # ищем функцию (5-я колонка - sfuncname)
    for row in data:
        if len(row) >= 5 and row[4] == query:
            print('\t'.join(row))
            break
    else:
        print("Функция не найдена")