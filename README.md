# Digital_voltmeter
Multi-channel digital voltmeter for test task

## Особенности реализации
Проект реализован для отладочной платы Blue Pill от STM. 
На плате находится МК STM32F103C8, присутствует кварцевый резонатор на 8МГц, а также светодиод индикации, подсоединенный к выводу C13. Логика светодиода инверсная.
Во время инициализации происходит переключение на тактирование от внешнего кварца, через PLL с умножением на 9. Итоговая частота системного таймера равна 72МГц.
Из сторонних библиотек только CMSIS и ОС FreeRTOS. Все остальное написано с нуля.
Конфигурация линковщика нестандартная, размер стека = 0x800, размер кучи = 0x1200 (файл raw_freertos_stm32103c8.icf уже активирован в настройках IAREW проекта)
Модифицирован файл CMSIS/src/system_stm32f1xx.c  (раскомментирована строчка #define USER_VECT_TAB_ADDRESS для верного указания адреса таблицы векторов прерываний: с адреса 0x08000000 (FLASH_BASE) )
Модифицирован файл CMSIS/src/startup_stm32f103xb.s ( переопределен переход на ассемблерные функции FreeRTOS vPortSVCHandler, xPortPendSVHandler, xPortSysTickHandler по соответствующим прерываниям )
Внимание! Согласно документации, размер Flash памяти МК STM32F103C8 равен 64Kb, однако на большинстве этих МК (и на моем) размер flash 128Kb. Прошивка, даже при максимальной оптимизации компилятора, занимает 90Kb. Она помещается на МК на моей плате, однако есть вероятность, что другой аналогичный МК будет иметь меньший размер flash памяти.
Использовалать IDE IAR Embedded Workbench - Arm 9.30.1

## Сборка
Необходимо открыть файл проекта IrkutskTask.eww в среде IAR Embedded Workbench - Arm, после этого собрать проект как обычно.

## Работа. Система команд
По умолчанию используются Uart1 и Adc1. Предусмотрена возможность 

## Краткое описание работы модулей
### stm32uart

### stm32adc

### led_blinker

### Voltmeter
UART
Uart выполнен в виде отдельного независимого от остальных частей модуля (см. папку stm32uart).
Модуль также разделен на логическую (абстрактную) часть и реализацию для конкретной платформы.
Файл stm32uart_port.cpp это единственная единица трансляции модуля, зависящая от платформы.
Соответственно, для портирования можно переопределить функции, объявленные в данном файле, в соответствии с целевой платформой.
Uart реализован с использованием кольцевого буфера и DMA в кольцевом режиме.
Размер буфера задается на этапе компиляции. Счетчик DMA равен размеру буфера.
Как только приемник uart получает байт, DMA канал получает соответствующий сигнал и перемещает данный байт в ячейку буфера, счетчик DMA аппаратно уменьшается на единицу.
Когда необходимо забрать данные из буфера, считывается регистр счетчика DMA. Значение счетчика сравнивается с аналогичным предыдущим значением (которое хранится). 
Если текущее и предыдущее значения счетчика DMA не совпадают, значит, есть непрочитанные данные. Известны адреса начала и конца непрочитанных данных в буфере, так что данные можно забрать.
После прочтения хранимое значение счетчика DMA перезаписывается на текущее.
Необходимо достаточно часто проверять буфер, чтобы данные не затерлись. Эту проблему можно решить при помощи прерываний DMA при необходимости.
После прочтения данные обрабатываются, чтобы отбросить ошибочные сообщения. Ошибочным считается сообщение больше некоторой длины, не оканчивающееся спец. символом окончания сообщения (например, '\n'). Символ задается в файле конфигурации.
Для отправки сообщений также используется DMA и буфер.
Интерфейс UART описан в файле stm32uart.h

ADC
Adc построен по принципу, описанному выше для uart для облегчения портирования.
Adc использует DMA и буфер. Одновременно активных каналов может быть несколько. Каналы можно добавлять в скан-лист и убирать их из него. 
Интерфейс АЦП описан в файле stm32adc.h

При добавлении очередного канала в скан-лист счетчик DMA увеличивается на 1, а также выбранный канал добавляется в regular channels ADC.
Как только очередной канал измерен, DMA получает сигнал и перемещает данные в буфер, в ячейку, соответствующую этому каналу.
Замеры идут непрерывно (continious / scan mode).
Актуальные значения каналов, таким образом, все время хранятся в буфере.
Соответствие канала и ячейки буфера хранится внутри специального класса AdcManager.

Voltmeter
Реализован в виде класса, все поля и методы которого статические. Управление происходит через команды-сообщения.
Возможно добавление новых команд и модификация уже существующих. 
Хранит список активных каналов и берет на себя работу по взаимодействию с каналами Adc, расположенными ниже по уровню абстракции.
Каналы имеют общий интерфейсный класс, от которого наследуются конкретные типы каналов (например, мгновенное значение, среднее, среднеквадратическое)
Таким образом, легко добавить или модифицировать тип канала.
По запросу значения АЦП преобразуются к вольтам. Можно настроить пределы, изменив соответствующие коэффициенты. 
Легко реализовать динамическое изменение значений при помощи uart команд.

Реализация различных типов каналов
Мгновенное значение.
Не хранит никаких значений, вместо этого по запросу возвращается актуальное значение канала, приведенное к вольтам

Среднее
Хранит n последних значений канала, измеренных через промежутки времени t. (Например, 20 значений через каждые 2мс)
Значения обновляются по таймеру. Используется программный таймер FreeRTOS, хотя это и неоптимальное решение.
По запросу ищет максимальное и минимальное значения, находит среднее, приводит к вольтам и возвращает.
Легко модифицировать команды uart, чтобы можно было динамически менять n и t. (Это уже реализовано в конструкторе канала)

Среднеквадратическое.
Все так же, как и для среднего значения, но среднее значение вычитается из максимального и домножается на кв корень из 2

Данная реализация, хоть и неоптимальная, обеспечивает относительную погрешность около 0.5% (0.01-0.02В при диапазоне 3.3В)
