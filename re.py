import sys

# Читаем таблицу из файла
with open(sys.argv[1], 'r') as f:
    data = [line.strip().split('\t') for line in f if line.strip()]

# Спрашиваем что искать
query = input("Что ищем? (dll или функция): ")

if '.dll' in query.lower():  # ищем DLL
    funcs = [line for line in data if len(line) == 3 and line[1].lower() == query.lower()]
    with open(f'{query}_functions.txt', 'w') as out:
        for f in funcs:
            out.write('\t'.join(f) + '\n')
    print(f"Найдено {len(funcs)} функций, сохранено в {query}_functions.txt")
else:  # ищем функцию
    for line in data:
        if len(line) == 3 and line[2] == query:
            print('\t'.join(line))
            break
    else:
        print("Функция не найдена")