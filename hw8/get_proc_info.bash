#!/bin/bash

declare -a descriptions
declare -a values

declare -A process_status_to_info=(
  ["R"]="Работает"
  ["S"]="Спит"
  ["D"]="Непрерывно спит"
  ["Z"]="Зомби"
  ["T"]="Остановлен"
)

add_param() {
  descriptions+=("$1")
  values+=("$2")
}

format_bytes_pretty() {
    local bytes=$1
    local literal_num=${2:0}
    local literals=("B" "KB" "MB" "GB")

    while (( bytes >= 1024 * 10 && literal_num < ${#literals[@]} - 1 )); do
        bytes=$((bytes / 1024))
        ((literal_num++))
    done

    echo "${bytes} ${literals[literal_num]}"
}

while true; do
  clear
  unset descriptions
  unset values

  read PID cpu_usage <<< $(ps aux --sort=-%cpu | awk 'NR==2 {print $2, $3}')

  if [[ ! -d /proc/$PID ]]; then
    echo "Процесс неожиданно завершился, попытка найти новый..."
    continue
  fi

  # Я старался брать информацию не из status там, где понимал, как это сделать...

  add_param "PID" "$PID"
  add_param "Использование CPU" "$cpu_usage%"
  add_param "Имя исполняемого файла" "$(cat /proc/$PID/comm)"
  add_param "Статус" "${process_status_to_info[$(awk '{print $3}' /proc/$PID/stat)]}"
  add_param "PID родительского процесса" "$(awk '{print $4}' /proc/$PID/stat)"
  add_param "Уровень приоритета" "$(awk '{print $18}' /proc/$PID/stat)"
  add_param "Путь к исполняемому файлу" "$(readlink -f /proc/$PID/exe)"
  add_param "Рабочая директория процесса" "$(readlink -f /proc/$PID/cwd)"
  add_param "Корневая директория процесса" "$(readlink -f /proc/$PID/root)"
  add_param "Общее количество прочитанных байт" "$(format_bytes_pretty "$(awk '/rchar/ {print $2}' /proc/$PID/io)")"
  add_param "Общее количество записанных байт" "$(format_bytes_pretty "$(awk '/wchar/ {print $2}' /proc/$PID/io)")"
  add_param "Общий объем использованной памяти" "$(format_bytes_pretty "$(awk '/VmSize/ {print $2}' /proc/$PID/status)" 1)"
  add_param "Максимальный объем использованной памяти" "$(format_bytes_pretty "$(awk '/VmPeak/ {print $2}' /proc/$PID/status)" 1)"
  add_param "Количество потоков" "$(awk '/Threads/ {print $2}' /proc/$PID/status)"

  for i in "${!values[@]}"; do
    printf "%s\t%s\n" "${descriptions[$i]}" "${values[$i]}"
  done | column -t -s $'\t'

#  printf "\n\nФайл /status:\n\n"
#
#  cat /proc/$PID/status | column -t -s $'\t'

  sleep 5
done
