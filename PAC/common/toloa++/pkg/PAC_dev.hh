/// @file PAC_dev.hh
/// @brief Классы и функции, доступные в Lua.
///
/// @par Текущая версия:
/// @$Rev$.\n
/// @$Author$.\n
/// @$Date::                     $.

$#include <stdlib.h>

$#include "PAC_dev.h"
$#include "tech_def.h"
$#include "cip_tech_def.h"
$#include "bus_coupler_io.h"
$#include "PID.h"
$#include "g_device.h"
$#include "g_errors.h"

$#include "modbus_client.h"

$#include "modbus_serv.h"

$#include "profibus_slave.h"

$#ifdef WIN_OS
$#pragma warning(disable: 4800) //Warning C4800: 'int' : forcing value to bool 'true' or 'false' (performance warning)
$#pragma warning(disable: 6011)  //dereferencing NULL pointer <name>.
$#pragma warning(disable: 26812) //Prefer 'enum class' over 'enum'.
$#endif // WIN_OS

//-----------------------------------------------------------------------------
/// @brief Устройство на основе дискретного входа.
///
/// Обратная связь, предельный уровень и т.д. являются примерами таких
/// устройств.
class i_DI_device
    {
    public:
        /// @brief Получение отфильтрованного состояния устройства.
        ///
        /// Устройство меняет свое состояние, если дискретный вход находится в
        /// в данном состоянии больше заданного интервала времени.
        ///
        /// @return - состояние устройства в виде целого числа.
        int get_state();

        /// @brief Получение логического активного состояния устройства.
        bool is_active();
    };
//-----------------------------------------------------------------------------
/// @brief Устройство на основе дискретного выхода.
///
/// Клапан, мешалка и т.д. являются примерами таких устройств.
class i_DO_device: public i_DI_device
    {
    public:
        /// @brief Включение устройства.
        ///
        /// Установка устройства в активное состояние. Для клапана это означает
        /// его активирование, то есть если он нормально закрытый - открытие.
        void on();

        /// @brief Выключение устройства.
        ///
        /// Установка устройства в пассивное состояние. Для клапана это означает
        /// его деактивирование, то есть если он нормально закрытый - закрытие.
        void off();

        /// @brief немедленное выключение устройства
        ///
        /// Устарело, для обратной совместимости.
        void instant_off @ direct_off();

        /// @brief немедленное выключение устройства c учетом ручного режима
        void instant_off();

        /// @brief Установка нового состояния устройства.
        ///
        /// @param new_state - новое состояние объекта.
        void set_state( int new_state );
    };
//-----------------------------------------------------------------------------
/// @brief Устройство на на основе аналогового входа.
///
/// Температура, расход и т.д. являются примерами таких устройств.
class i_AI_device
    {
    public:
        /// @brief Получение текущего состояния устройства.
        ///
        /// @return - текущее состояние устройства в виде дробного числа.
        float get_value();

        /// @brief Получение состояния устройства.
        ///
        /// @return состояние устройства в виде целого числа.
        virtual int get_state();
    };
//-----------------------------------------------------------------------------
/// @brief Устройство на основе аналогового выхода.
///
/// Аналоговый канал управления и т.д. являются примерами таких устройств.
class i_AO_device: public i_AI_device
    {
    public:
        /// @brief Выключение устройства.
        ///
        /// Установка устройства в пассивное состояние. Для клапана это означает
        /// его деактивирование, то есть если он нормально закрытый - закрытие.
        void off();

        /// @brief Установка текущего состояния устройства.
        ///
        /// @param new_value - новое состояние устройства.
        void set_value( float new_value );
    };
//-----------------------------------------------------------------------------
/// @brief Интерфейс устройства как с аналоговыми, так и дискретными каналами.
class i_DO_AO_device: public i_DO_device, public i_AO_device
    {
    public:
        /// @brief Получение отфильтрованного состояния устройства.
        ///
        /// Устройство меняет свое состояние, если дискретный вход находится в
        /// в данном состоянии больше заданного интервала времени.
        ///
        /// @return - состояние устройства в виде целого числа.
        int get_state();

        /// @brief Установка состояния.
        ///
        /// Данный метод используется для задания состояния устройства перед
        /// его проверкой.
        ///
        /// @param new_state - новое состояние.
        void set_state( int new_state );

        /// @brief Включение устройства.
        ///
        /// Установка устройства в активное состояние. Для клапана это означает
        /// его активирование, то есть если он нормально закрытый - открытие.
        void on();

        /// @brief Выключение устройства.
        ///
        /// Установка устройства в пассивное состояние. Для клапана это означает
        /// его деактивирование, то есть если он нормально закрытый - закрытие.
        void off();

        /// @brief Получение текущего состояния устройства.
        ///
        /// @return - текущее состояние устройства в виде дробного числа.
        float get_value();

        /// @brief Установка текущего состояния устройства.
        ///
        /// @param new_value - новое состояние устройства.
        void set_value( float new_value );
    };
//-----------------------------------------------------------------------------
/// @brief Интерфейс счетчика.
class i_counter
    {
    public:
        /// @brief Приостановка работы счетчика.
        void pause();

        /// @brief Возобновление работы счетчика.
        void start();

        /// @brief Сброс счетчика.
        ///
        /// После сброса для продолжения работы необходимо вызвать @ref start().
        void reset();

        /// @brief Сброс счетчика и продолжение счета.
        void restart();

        /// @brief Получение значения счетчика (объем).
        unsigned int get_quantity();

        /// @brief Получение значения счетчика (поток).
        float get_flow();

        /// @brief Получение состояния работы счетчика.
        virtual int get_state();

        /// @brief Получение абсолютного значения счетчика (без учета
        /// состояния паузы).
        unsigned int get_abs_quantity();

        /// @brief Сброс абсолютного значения счетчика.
        void abs_reset();
    };
//-----------------------------------------------------------------------------
/// @brief Простое физическое устройство.
///
/// Примеры таких устройств: клапан, насос, мешалка и т.д.
class device : public i_DO_AO_device
    {
#pragma region Управление устройством из пользовательского скрипта Lua.
    public:
        /// @brief Выключение устройства с учетом ручного режима.
        void off();

        /// @brief Включение устройства с учетом ручного режима.
        void on();

        /// @brief Установка нового состояния устройства с учетом ручного режима.
        ///
        /// @param new_state - новое состояние устройства.
        void set_state( int new_state );

        /// @brief Получение текущего состояния устройства.
        ///
        /// @return - текущее состояние устройства в виде дробного числа.
        float get_value();

        /// @brief Установка текущего состояния устройства.
        ///
        /// @param new_value - новое состояние устройства.
        void set_value( float new_value );
#pragma endregion

    public:
        /// @brief Выполнение команды устройством.
        ///
        /// Для обработки команд, полученных от сервера.
        int set_cmd( const char *prop, unsigned int idx, double val );

        void set_par( unsigned int idx, unsigned int offset, float value );

        /// @brief Установка значения рабочего параметра.
        ///
        /// @param idx - индекс рабочего параметра (с единицы).
        /// @param value - новое значение.
        virtual void set_rt_par( unsigned int idx, float value );

        void set_property( const char* field, device* dev );

        void set_string_property(const char* field, const char* value);

        void set_descr( const char *description );

        const char* get_type_str() const;

        enum DEVICE_TYPE
            {
            DT_NONE = -1,///< Тип не определен.

            DT_V = 0,   ///< Клапан.
            DT_VC,      ///< Управляемый клапан.
            DT_M,       ///< Двигатель.
            DT_LS,      ///< Уровень (есть/нет).
            DT_TE,      ///< Температура.
            DT_FS,      ///< Расход (есть/нет).
            DT_GS,      ///< Датчик положения.
            DT_FQT,     ///< Счетчик.
            DT_LT,      ///< Уровень (значение).
            DT_QT,      ///< Концентрация.

            DT_HA,      ///< Аварийная звуковая сигнализация.
            DT_HL,      ///< Аварийная световая сигнализация.
            DT_SB,      ///< Кнопка.
            DT_DI,      ///< Дискретный входной сигнал.
            DT_DO,      ///< Дискретный выходной сигнал.
            DT_AI,      ///< Аналоговый входной сигнал.
            DT_AO,      ///< Аналоговый выходной сигнал.
            DT_WT,		///< Тензорезистор.
            DT_PT,      ///< Давление (значение).
            DT_F,       ///< Автоматический выключатель.
            };
    };
//-----------------------------------------------------------------------------
/// @brief Весы
class i_wages
    {
    public:
        /// @brief Тарировка.
        ///
        /// @return - none.
        void tare();

        /// @brief Получение текущего состояния устройства.
        ///
        /// @return - Вес в кг.
        float get_value();
    };
//-----------------------------------------------------------------------------
/// @brief Мотор.
class i_motor : public device
    {
    public:
        /// @brief Включение мотора в реверсном направлении.
        void reverse();

        /// @brief Получение линейной скорости (например, приводимого в
        // движение конвейра).
        virtual float get_linear_speed() const;

        /// @brief Получение текущего тока мотора
        virtual float get_amperage() const;
    };
//-----------------------------------------------------------------------------
class signal_column : public device
    {
    public:
        /// @brief Выключение устройства с учетом ручного режима.
        void off();

        /// @brief Включение устройства с учетом ручного режима.
        void on();

        /// @brief Установка нового состояния устройства с учетом ручного режима.
        ///
        /// @param new_state - новое состояние устройства.
        void set_state( int new_state );

        /// @brief Получение текущего состояния устройства.
        ///
        /// @return - текущее состояние устройства в виде дробного числа.
        float get_value();

        /// @brief Установка текущего состояния устройства.
        ///
        /// @param new_value - новое состояние устройства.
        void set_value( float new_value );

        void turn_off_red();
        void turn_off_yellow();
        void turn_off_green();
        void turn_off_blue();

        void turn_on_red();
        void turn_on_yellow();
        void turn_on_green();
        void turn_on_blue();

        void normal_blink_red();
        void normal_blink_yellow();
        void normal_blink_green();
        void normal_blink_blue();

        void slow_blink_red();
        void slow_blink_yellow();
        void slow_blink_green();
        void slow_blink_blue();

        void turn_on_siren();
        void turn_off_siren();

        enum STATE
            {
            TURN_OFF,

            TURN_ON,

            LIGHTS_OFF,

            GREEN_ON,
            YELLOW_ON,
            RED_ON,

            GREEN_OFF,
            YELLOW_OFF,
            RED_OFF,

            GREEN_NORMAL_BLINK,
            YELLOW_NORMAL_BLINK,
            RED_NORMAL_BLINK,

            GREEN_SLOW_BLINK,
            YELLOW_SLOW_BLINK,
            RED_SLOW_BLINK,

            SIREN_ON,
            SIREN_OFF,
            };

#ifdef _MSC_VER
#pragma region Сигнализация о событиях
#endif
        void show_error_exists();
        void show_message_exists();

        void show_batch_is_not_running();
        void show_batch_is_running();

        void show_operation_is_not_running();
        void show_operation_is_running();

        void show_idle();
#ifdef _MSC_VER
#pragma endregion
#endif
    };
//-----------------------------------------------------------------------------
/// @brief Получение клапана по имени.
///
/// @param dev_name - имя клапана.
/// @return - клапан с заданным именем. Если нет такого клапана, возвращается
/// заглушка (@ref dev_stub).
valve* V( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение управляемого клапана по имени.
///
/// @param dev_name - имя клапана.
/// @return - клапан с заданным именем. Если нет такого клапана, возвращается
/// заглушка (@ref dev_stub).
i_AO_device* VC( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение двигателя по имени.
///
/// @param dev_name - имя двигателя.
/// @return - двигатель с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_motor* M( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение сигнального уровня по имени.
///
/// @param dev_name - имя сигнального уровня.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_DI_device* LS( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение сигнального расхода по имени.
///
/// @param dev_name - имя сигнального расхода.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_DI_device* FS( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение аналогового входа по имени.
///
/// @param dev_name - имя аналогового входа.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_AI_device* AI( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение аналогового выхода по имени.
///
/// @param dev_name - имя аналогового выхода.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_AO_device* AO( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение счетчика по имени.
///
/// @param dev_name - имя счетчика.
/// @return - устройство с заданным номером. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_counter* FQT( const char *dev_name );

virtual_counter* virtual_FQT( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение температуры по имени.
///
/// @param dev_name - имя температуры.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_AI_device* TE( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение текущего уровня по имени.
///
/// @param dev_name - имя текущего уровня.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
level* LT( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение датчика положения по имени.
///
/// @param dev_name - имя датчика положения.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_DI_device* GS( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение звуковой сигнализации по имени.
///
/// @param dev_name - имя звукововй сигнализации.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_DO_device* HA( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение световой сигнализации по имени.
///
/// @param dev_name - имя световой сигнализации.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_DO_device* HL( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение сигнальной колонны по имени.
///
/// @param dev_name - имя сигнальной колонны.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
signal_column* HLA( const char* dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение кнопки по имени.
///
/// @param dev_name - имя кнопки.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_DI_device* SB( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение обратной связи по имени.
///
/// @param dev_name - имя обратной связи.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_DI_device* DI( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение канала управления по имени.
///
/// @param dev_name - имя канала управления.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_DO_device* DO( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение текущей концентрации по имени.
///
/// @param dev_name - имя текущей концентрации.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_AI_device* QT( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение весов по имени.
///
/// @param dev_name - имя текущих весов.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_wages* WT( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение текущего давления по имени.
///
/// @param dev_name - имя текущего давления.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_AI_device* PT( const char *dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение автоматического выключателя по имени.
///
/// @param number - номер автоматического выключателя.
/// @return - устройство с заданным именем. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
i_DO_AO_device* F(const char* dev_name);
//-----------------------------------------------------------------------------
/// @brief Получение регулятора по имени.
///
/// @param dev_name - имя.
/// @return - устройство с заданным номером. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
PID* C( const char* dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение камеры по имени.
///
/// @param dev_name - имя.
/// @return - устройство с заданным номером. Если нет такого устройства,
/// возвращается заглушка (@ref dev_stub).
camera* CAM( const char* dev_name );
//-----------------------------------------------------------------------------
/// @brief Получение устройства-заглушки.
///
/// @return - виртуальное устройство.
dev_stub* STUB();

device* DEVICE( int s_number );
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/// @brief Виртуальное устройство.
///
/// Необходимо для возвращения результата поиска устройства с несуществующим
/// номером. Методы данного класса ничего не делают.
class dev_stub
    {
    public:
        /// @brief Получение состояния устройства.
        ///
        /// @return 0
        float get_value();

        /// @brief Установка состояния устройства. Ничего не делает.
        void set_value( float new_value );


        /// @brief Включение устройства. Ничего не делает.
        void on();

        /// @brief Выключение устройства. Ничего не делает.
        void off();


        /// @brief Установка состояния устройства. Ничего не делает.
        void set_state( int new_state );

        /// @brief Получение состояния устройства.
        ///
        /// @return 0
        int get_state();

        /// @brief Остановка счетчика. Ничего не делает.
        void pause();

        /// @brief Возобновление счетчика. Ничего не делает.
        void start();

        /// @brief Сброс счетчика. Ничего не делает.
        void reset();

        /// @brief Получение значения счетчика.
        ///
        /// @return 0
        unsigned int get_quantity();
    };
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/// @brief Содержит информацию об всех ошибках простых устройств.
class errors_manager
    {
    public:
        /// @brief Получение единственного экземпляра класса.
        static errors_manager* get_instance();

        /// @brief Изменение параметров ошибки.
        void set_cmd( unsigned int cmd, unsigned int object_type,
            unsigned int object_number, unsigned int object_alarm_number );
    };

//Совместимость с предыдущей версией драйвера EasyDrv. FIXME.
class dev_errors_manager
    {
    public:
        /// @brief Получение единственного экземпляра класса.
        static errors_manager* get_instance();

        /// @brief Изменение параметров ошибки.
        void set_cmd( unsigned int cmd, unsigned int object_type,
            unsigned int object_number, unsigned int object_alarm_number );
    };
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/// @brief Интерфейс valve.
class valve
    {
    public:
        /// @brief Получение значения включенного состояния.
        bool is_opened();

        /// @brief Получение значения выключенного состояния.
        bool is_closed();

        /// @brief Включение устройства с учетом ручного режима.
        virtual void on();

        /// @brief Выключение устройства с учетом ручного режима.
        virtual void off();

        /// @brief немедленное выключение устройства
        ///
        /// Устарело, для обратной совместимости.
        void instant_off @ direct_off();

        /// @brief немедленное выключение устройства c учетом ручного режима
        virtual void instant_off();

        /// @brief Установка нового состояния устройства с учетом ручного режима.
        ///
        /// @param new_state - новое состояние устройства.
        virtual void set_state( int new_state );

        /// @brief Получение состояния устройства.
        ///
        /// @return состояние устройства в виде целого числа.
        virtual int get_state();

    public:
        /// @brief Получение значения обратной связи на включенное состояние.
        int get_on_fb_value();

        /// @brief Получение значения обратной связи на выключенное состояние.
        int get_off_fb_value();

    ///Состояние клапана без учета обратной связи.
    enum VALVE_STATE
        {
        V_LOWER_SEAT = 3, ///< Открыто нижнее седло.
        V_UPPER_SEAT = 2, ///< Открыто верхнее седло.

        V_ON  = 1,        ///< Включен.
        V_OFF = 0,        ///< Выключен.
        };
    };
//-----------------------------------------------------------------------------
/// @brief Интерфейс текущего уровеня.
class level : public i_AI_device
    {
    public:
        virtual float get_volume();
    };
//-----------------------------------------------------------------------------
/// @brief Виртуальное устройство без привязки к модулям ввода-вывода
class virtual_counter : public device
    {
    public:
        /// @brief Сброс счетчика.
        void reset();

        /// @brief Получение значения счетчика (объем).
        unsigned int get_quantity();

        /// @brief Получение значения счетчика (поток).
        float get_flow();

        /// @brief Получение состояния работы счетчика.
        virtual int get_state();

        /// @brief Получение абсолютного значения счетчика (без учета
        /// состояния паузы).
        unsigned int get_abs_quantity();

        /// @brief Сброс абсолютного значения счетчика.
        void abs_reset();

        void set( unsigned int value, unsigned int abs_value, float flow );

        void eval( unsigned int read_value, unsigned int abs_read_value,
            float read_flow );
    };
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/// @brief Работа с технологическим объектом.
///
/// Базовый класс для технологического объекта (танка, гребенки). Содержит
/// основные методы работы - работа с режимами и т.д.
class tech_object
    {
    public:
        int set_cmd( const char *prop, unsigned int idx, double val );

        /// @param name                     - название ("Гребенка", ...).
        /// @param number                   - номер.
        /// @param type                     - тип.
        /// @param name_Lua                 - имя в Lua.
        /// @param states_count             - количество режимов.
        /// @param timers_count             - количество таймеров.
        /// @param par_float_count          - количество сохраняемых параметров типа float.
        /// @param runtime_par_float_count  - количество рабочих параметров типа float.
        /// @param par_uint_count           - количество сохраняемых параметров типа uint.
        /// @param runtime_par_uint_count   - количество рабочих параметров типа uint.
        tech_object( const char* name, unsigned int number, unsigned int type,
            const char* name_Lua,
            unsigned int  states_count,
            unsigned int  timers_count,
            unsigned int  par_float_count, unsigned int  runtime_par_float_count,
            unsigned int  par_uint_count, unsigned int runtime_par_uint_count );

        ~tech_object();

        /// @brief Включение/выключение режима.
        ///
        /// @param mode      - режим.
        /// @param new_state - новое состояние режима. Принимает значения: 0 -
        /// выключить, 1 - включить, -1 - выключить без проверки на возможность
        ///	отключения.
        int set_mode( unsigned int mode, int new_state );

        /// @brief Получение состояния режима.
        ///
        /// @param mode - режим.
        ///
        /// @return 1 - режим включен.
        /// @return 0 - режим не включен.
        int get_mode( unsigned int mode );

        /// @brief Получение состояния режима.
        ///
        /// @param mode - режим.
        ///
        /// @return ... - режим в ...
        /// @return 2 - режим в паузе.
        /// @return 1 - режим включен.
        /// @return 0 - режим не включен.
        int get_operation_state( unsigned int mode );

        /// @brief Выполнение команды.
        ///
        /// Здесь могут выполняться какие-либо действия (включаться/выключаться
        /// другие режимы, включаться/выключаться какие-либо устройства).
        int exec_cmd( unsigned int cmd );

        /// @brief Получение числа режимов технологического объекта.
        ///
        /// Здесь могут выполняться какие-либо действия (включаться/выключаться
        /// другие режимы, включаться/выключаться какие-либо устройства).
        unsigned int get_modes_count() const;

        int check_operation_on( unsigned int operation_n, bool show_error = true);

        saved_params_float      par_float;   ///< Сохраняемые пар-ры, тип float.
        run_time_params_float   rt_par_float;///< Рабочие параметры, тип float.
        saved_params_u_int_4    par_uint;    ///< Сохраняемые пар-ры, тип uint.
        run_time_params_u_int_4 rt_par_uint; ///< Рабочие пар-ры, тип uint.

        timer_manager           timers;		 ///< Таймеры объекта.

        operation_manager* get_modes_manager();

        /// @brief Запрос отсутствия выполняющихся режимов.
        ///
        bool is_idle() const;

        int get_number() const;

        enum ERR_MSG_TYPES
            {
            ERR_CANT_ON,
            ERR_ON_WITH_ERRORS,
            ERR_OFF,
            ERR_OFF_AND_ON,
            ERR_DURING_WORK,
            ERR_ALARM,

            ERR_TO_FAIL_STATE,
            ERR_CANT_ON_2_OPER, //Уже включена блокирующая операция.
            ERR_CANT_ON_2_OBJ,  //Уже включена блокирующая операция другого объекта.
            };

        int set_err_msg( const char *err_msg, int mode, int new_mode = 0,
            ERR_MSG_TYPES type = ERR_CANT_ON );

        /// @brief Наличие активных событий.
        ///
        bool is_any_message() const;

        /// @brief Наличие активных аварий.
        ///
        bool is_any_error() const;
    };
//-----------------------------------------------------------------------------
///@brief Получение менеджера технологических объектов.
///
///@return Менеджер технологических объектов проекта.
tech_object_manager* G_TECH_OBJECT_MNGR();
//-----------------------------------------------------------------------------
/// @brief Менеджер технологических объектов.
///
/// Содержит информацию обо всех технологических объектах проекта.
class tech_object_manager
    {
    public:
        /// @brief Получение объекта с заданным активным режимом.
        int get_object_with_active_mode( unsigned int mode,
            unsigned int start_idx, unsigned int end_idx );

        /// @brief Получение технологического объекта по его порядковому номеру.
        tech_object* get_tech_objects( unsigned int idx );

        /// @brief Отладочная печать объекта.
        void print();
    };
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/// @brief Содержит информацию об операции.
///
/// Операция может находиться в одном из состояний.
class operation
    {
    public:
        enum state_idx
            {
            IDLE = 0,// Ожидание.
            RUN,     // Выполнение.
            PAUSE,   // Пауза.
            STOP,    // Остановлен.
            };

    public:
        unsigned long evaluation_time();

        unsigned int active_step() const;

        unsigned long active_step_evaluation_time() const;
        unsigned long get_active_step_set_time() const;

        /// @brief Переход к заданному шагу.
        ///
        /// @param new_step - номер шага (с единицы).
        /// @param cooperative_time - время совместной работы (сек).
        void to_step( unsigned int new_step, unsigned long cooperative_time = 0 );

        /// @brief Переход к следующему шагу.
        ///
        /// Данный метод упрощает реализацию, когда необходимо использовать
        /// функцию to_step, передавая в качестве параметра номер текущего шага,
        /// увеличенного на единицу.
        void to_next_step();

        /// @brief Отключение текущего активного шага.
        void turn_off_active_step();

        /// @brief Получение режима через операцию индексирования.
        ///
        /// @param idx - индекс режима.
        ///
        /// @return - значение режима с заданным индексом. Если индекс
        /// выходит за диапазон, возвращается значение заглушки - поля
        /// mode_stub.
        operation_state* operator[]( int idx );

    public:
        /// @brief Включение заданного шага.
        ///
        /// @param step - номер включаемого шага (с единицы).
        int on_extra_step( int step );

        /// @brief Отключение заданного шага.
        ///
        /// @param step - номер отключаемого шага (с единицы).
        int off_extra_step( int step );

        /// @brief Отключение заданного активного шага и включение другого.
        ///
        /// @param off_step - номер отключаемого шага (с единицы).
        /// @param on_step - номер включаемого шага (с единицы).
        int switch_active_extra_step( int off_step, int on_step );

        /// @brief Определение активности заданного шага.
        ///
        /// @param step_idx - номер выключаемого шага (с единицы).
        bool is_active_extra_step( int step_idx ) const;

    public:
        step* add_step( const char* name, int next_step_n,
            unsigned int step_duration_par_n, state_idx s_idx = RUN );

    public:

        state_idx get_state() const;
    };
//-----------------------------------------------------------------------------
/// @brief Содержит информацию о всех операциях какого-либо объекта (танк,
/// бачок и т.д.).
///
/// У объекта (танк, ...) операции включаются пользователем, системой и т.д.
class operation_manager
    {
    public:
        operation* add_mode( const char* name );
        operation* add_operation( const char* name );

        /// @brief Получение режима через операцию индексирования.
        ///
        /// @param idx - индекс режима.
        ///
        /// @return - значение режима с заданным индексом. Если индекс
        /// выходит за диапазон, возвращается значение заглушки - поля
        /// mode_stub.
        operation* operator[] ( unsigned int idx );

        /// @brief Время бездействия (нет включенных операций).
        ///
        /// @return - время системы без активных операций.
        unsigned long get_idle_time();
    };
//-----------------------------------------------------------------------------
/// @brief Состояние операции.
///
/// Содержит группу шагов, выполняемых последовательно (или в ином порядке).
class operation_state
    {
    public:
        /// @brief Получение операции через операцию индексирования.
        ///
        /// @param idx - индекс операции.
        ///
        /// @return - значение операции с заданным индексом. Если индекс
        /// выходит за диапазон, возвращается значение заглушки - поля @ref
        /// mode::step_stub.
        step* operator[] ( int idx );

        unsigned int steps_count() const;
    };
//-----------------------------------------------------------------------------
/// @brief Содержит информацию об устройствах, которые входят в шаг (открываются/
/// закрываются).
///
/// У режима может быть активным (выполняться) только один шаг.
class step
    {
    public:
        /// @brief Получение действия через операцию индексирования.
        ///
        /// @param index - индекс действия.
        ///
        /// @return - значение действия с заданным индексом. Если индекс
        /// выходит за диапазон, возвращается значение 0.
        action* operator[] ( int idx );

        const char* get_name() const;

        enum ACTIONS
            {
            A_CHECKED_DEVICES = 0,
            A_ON,
            A_ON_REVERSE,
            A_OFF,
            A_UPPER_SEATS_ON,
            A_LOWER_SEATS_ON = A_UPPER_SEATS_ON,

            A_REQUIRED_FB,
            A_DI_DO,
            A_AI_AO,
            A_WASH,

            A_ENABLE_STEP_BY_SIGNAL,
            A_TO_STEP_IF,
            };

        bool is_active() const;
    };
//-----------------------------------------------------------------------------
/// @brief Действие над устройствами (включение, выключение и т.д.).
class action
    {
    public:
        /// @brief Добавление устройства к действию.
        ///
        /// @param [in] dev Устройство.
        /// @param [in] group Дополнительный параметр.
        /// @param [in] subgroup Дополнительный параметр.
        virtual void add_dev( device *dev, unsigned int group,
            unsigned int subgroup );

        /// @brief Задание индексов используемых параметров к действию.
        ///
        /// @param [in] position Позиция параметра.
        /// @param [in] idx Индекс параметра.
        void set_param_idx( unsigned int position, int idx );
    };
//-----------------------------------------------------------------------------
///@brief Получение менеджера устройств.
///
///@return Менеджер устройств проекта.
device_manager* G_DEVICE_MANAGER();

///@brief Получение технологического объекта по номеру.
///
///@return Технологический объект.
tech_object* G_TECH_OBJECTS( unsigned int idx );
//-----------------------------------------------------------------------------
/// @brief Менеджер устройств.
///
/// Содержит информацию обо всех устройствах проекта.
class device_manager
    {
    public:
        /// @brief Отладочная печать объекта в консоль.
        void print() const;

        // @brief Установка числа устройств.
        //
        // Вызывается из Lua.
        io_device* add_io_device( int dev_type, int dev_sub_type,
            const char *dev_name, char * descr, char* article );

        /// @brief Получение устройства по его номеру.
        device* get_device( int dev_type,
            const char *dev_name );
    };
//-----------------------------------------------------------------------------
/// @brief Устройство на основе модулей ввода/вывода.
///
/// В общем случае у устройства может быть один или несколько каналов
/// ввода/вывода (дискретных или аналоговых).
class io_device
    {
    public:
        // Lua.
        enum VENDOR
            {
            WAGO,
            PHOENIX,
            };

        void init( int DO_count, int DI_count,
            int AO_count, int AI_count );

        void init_channel( int type, int ch_inex, int node, int offset, int module_offset = -1, int logical_port = -1 );

        void set_io_vendor( VENDOR vendor );
    };
//-----------------------------------------------------------------------------
/// @brief Группа таймеров.
class timer_manager
    {
    public:
        /// @brief Безопасное получение таймера по индексу.
        ///
        /// @param index - индекс таймера.
        ///
        /// @return - таймер с нужным индексом, заглушка - в случае выхода за
        /// диапазон.
        timer* operator[] ( unsigned int index );
    };
//-----------------------------------------------------------------------------
/// @brief Таймер.
class timer
    {
    public:
        /// @brief Состояние таймера.
        enum STATE
            {
            S_STOP = 0, ///< Не работает.
            S_WORK,     ///< Работает.
            S_PAUSE,    ///< Пауза.
            };

        /// @brief Запуск таймера.
        void start();

        /// @brief Сброс таймера.
        void reset();

        /// @brief Пауза таймера.
        void pause();

        /// @brief Проверка исхода времени таймера.
        ///
        /// @return true  - время вышло.
        /// @return false - время не вышло.
        bool is_time_up() const;

        /// @brief Получение времени работы таймера.
        ///
        /// @return - время работы таймера.
        unsigned long  get_work_time() const;

        /// @brief Установка задания таймера.
        ///
        /// @param new_countdown_time - задание.
        void set_countdown_time( unsigned long new_countdown_time );

        /// @brief Получение задания таймера.
        ///
        /// @return - задание таймера.
        unsigned long  get_countdown_time() const;

        /// @brief Получение состояния таймера.
        ///
        /// @return - состояние таймера.
        STATE get_state() const;

        timer();
    };
//-----------------------------------------------------------------------------
/// @brief Работа с сохраняемыми параметрами типа "дробное число".
///
/// Параметры сохраняются в энергонезависимой памяти.
class saved_params_float
    {
    public:
        /// @brief Сохранение значения @a value в параметр с номером @a idx
        /// в энергонезависимой памяти.
        ///
        /// @param idx   - индекс параметра.
        /// @param value - значение параметра.
        int save( unsigned int idx, float value );

        /// @brief Сохранение всех параметров в энергонезависимой памяти.
        int save_all();

        /// @brief Получение параметра по его номеру.
        ///
        /// При присваивании параметру нового значения (например par[ 5 ] = 6),
        /// параметр сохраняется только в оперативной памяти (не сохранит
        ///	значение после перезагрузки контроллера).
        ///
        /// @param idx   - индекс параметра.
        ///
        /// @return - параметр c заданным индексом.
        float& operator[] ( unsigned int idx );

        /// @brief Установка всех параметров в нулевое значение и сохранение
        /// в энергонезависимой памяти.
        void reset_to_0();
    };
//-----------------------------------------------------------------------------
/// @brief Работа с сохраняемыми параметрами типа "беззнаковое целое число".
///
/// Параметры сохраняются в энергонезависимой памяти.
class saved_params_u_int_4
    {
    public:
        /// @brief Сохранение значения @a value в параметр с номером @a idx
        /// в энергонезависимой памяти.
        int save( unsigned int idx, unsigned int value );

        /// @brief Сохранение всех параметров в энергонезависимой памяти.
        int save_all();

        /// @brief Получение параметра по его номеру.
        ///
        /// При присваивании параметру нового значения (например par[ 5 ] = 6),
        /// параметр сохраняется только в оперативной памяти (не сохранит
        ///	значение после перезагрузки контроллера).
        ///
        /// @param idx   - индекс параметра.
        ///
        /// @return - параметр c заданным индексом.
        unsigned int& operator[] ( unsigned int idx );

        /// @brief Установка всех параметров в нулевое значение и сохранение
        /// в энергонезависимой памяти.
        void reset_to_0();
    };
//-----------------------------------------------------------------------------
/// @brief Работа с рабочими параметрами типа "беззнаковое целое число".
///
/// Параметры сохраняются в оперативной памяти.
class run_time_params_u_int_4
    {
    public:
        /// @brief Получение параметра по его номеру.
        ///
        /// @param idx - индекс параметра.
        ///
        /// @return - параметр c заданным индексом.
        unsigned int& operator[] ( unsigned int idx );

        /// @brief Установка всех параметров в нулевое значение.
        void reset_to_0();
    };
//-----------------------------------------------------------------------------
/// @brief Работа с рабочими параметрами типа "дробное число".
///
/// Параметры сохраняются в оперативной памяти.
class run_time_params_float
    {
    public:
        /// @brief Получение параметра по его номеру.
        ///
        /// @param idx - индекс параметра.
        ///
        /// @return - параметр c заданным индексом.
        float& operator[] ( unsigned int idx );

        /// @brief Установка всех параметров в нулевое значение.
        void reset_to_0();
    };
//-----------------------------------------------------------------------------
/// @brief Работа с модулями ввода/вывода.
///
/// Реализация чтения и записи состояний модулей ввода/вывода.
class io_manager
    {
    public:
        /// @brief Установка числа модулей.
        ///
        /// Вызывается из Lua.
        void init( int nodes_count );

        /// @brief Инициализация модуля.
        ///
        /// Вызывается из Lua.
        void add_node(  unsigned int index, int ntype, int address,
            char* IP_address, char *name, int DO_cnt, int DI_cnt, int AO_cnt, int AO_size,
            int AI_cnt, int AI_size );

        /// @brief Инициализация параметров канала аналогового вывода.
        ///
        /// Вызывается из Lua.
        void init_node_AO( unsigned int node_index, unsigned int AO_index,
            unsigned int type, unsigned int offset );

        /// @brief Инициализация параметров канала аналогового ввода.
        ///
        /// Вызывается из Lua.
        void init_node_AI( unsigned int node_index, unsigned int AI_index,
            unsigned int type, unsigned int offset );

    };
//-----------------------------------------------------------------------------
///@brief Получение менеджера.
///
///@return Менеджер устройств проекта.
io_manager* G_IO_MANAGER();
//-----------------------------------------------------------------------------
///@brief ПИД-регулятор.
///
///
class PID
    {
    public:

        ///@brief Основные сохраняемые параметры.
        enum PARAM
            {
            P_k = 1,               ///< Параметр k.
            P_Ti,                  ///< Параметр Ti.
            P_Td,                  ///< Параметр Td.
            P_dt,                  ///< Интервал расчёта
            P_max,                 ///< Мax значение входной величины.
            P_min,                 ///< Мin значение входной величины.
            P_acceleration_time,   ///< Время выхода на режим регулирования.
            P_is_manual_mode,      ///< Ручной режим.
            P_U_manual,            ///< Заданное ручное значение выходного сигнала.

            P_k2,                  ///< Параметр k2.
            P_Ti2,                 ///< Параметр Ti2.
            P_Td2,                 ///< Параметр Td2.

            P_out_max,             ///< Мax значение выходной величины.
            P_out_min,             ///< Мin значение выходной величины.
            };

        ///@brief Рабочие параметры.
        enum WORK_PARAM
            {
            WP_Z = 1,  ///< Требуемое значение.
            WP_U,      ///< Выход ПИД.
            };

        /// @param n - номер.
        PID( int n );

        ///@brief Включение регулятора.
        void on( char is_down_to_inaccel_mode = 0 );

        ///@brief Выключение регулятора.
        void  off();

        /// @brief Сброс ПИД
        void reset();

        ///@brief Получение следующего значения выхода на основе текущего
        /// значения входа.
        float eval( float current_value, int delta_sign = 1 );

        ///@brief Задание новое задание ПИД.
        void  set( float new_z );

        /// @brief Получение задания ПИД.
        float get_assignment();

        /// @brief Запись сохраняемого параметра.
        void init_param( PARAM par_n, float val );

        /// @brief Запись рабочего параметра.
        void init_work_param( WORK_PARAM par_n, float val );

        /// @brief Запись сохраняемых параметров в EEPROM.
        void save_param();

        /// @brief Задание для использования дополнительных P_k2, P_Ti2, P_Td2.
        void  set_used_par ( int par_n );

        /// @brief Отладочная печать объекта в консоль.
        void print();

        /// @brief Состояние регулятора.
        unsigned int get_state();

        int set_cmd( const char *prop, unsigned int idx, double val );
    };
//-----------------------------------------------------------------------------
/// @brief Камера.
///
/// Служит для получения событий о распозновании объекта.
class camera
    {
    public:
        /// @brief Выключение устройства с учетом ручного режима.
        void off();

        /// @brief Включение устройства с учетом ручного режима.
        void on();

        /// @brief Установка нового состояния устройства с учетом ручного режима.
        ///
        /// @param new_state - новое состояние устройства.
        void set_state( int new_state );

        /// @brief Получение текущего состояния устройства.
        ///
        /// @return - текущее состояние устройства в виде дробного числа.
        float get_value();

        /// @brief Установка текущего состояния устройства.
        ///
        /// @param new_value - новое состояние устройства.
        void set_value( float new_value );

        /// @brief Получение состояния устройства.
        ///
        /// @return состояние устройства в виде целого числа.
        int get_state();

        /// @brief Получение статуса событий от камеры.
        int get_result( int n = 1 );

        /// @brief Получение состояние готовности.
        bool is_ready() const;
    }
//-----------------------------------------------------------------------------
class PAC_info: public i_Lua_save_device
    {
    public:
        enum PARAMETERS
            {
            P_MIX_FLIP_PERIOD = 1, ///< Интервал промывки седел клапанов, сек.
            P_MIX_FLIP_UPPER_TIME, ///< Время промывки верхних седел клапанов, мсек.
            P_MIX_FLIP_LOWER_TIME, ///< Время промывки нижних седел клапанов, мсек

            P_V_OFF_DELAY_TIME,    ///< Время задержки закрытия клапанов, мсек.

            ///< Время задержки закрытия для донных клапанов, мсек.
            P_V_BOTTOM_OFF_DELAY_TIME,

            ///< Среднее время задержки получения ответа от узла, мсек.
            P_WAGO_TCP_NODE_WARN_ANSWER_AVG_TIME,
            ///< Среднее время цикла программы, мсек.
            P_MAIN_CYCLE_WARN_ANSWER_AVG_TIME,

            ///< Работа модуля ограничений.
            /// 0 - авто, 1 - ручной, 2 - полуручной (через время
            /// @P_RESTRICTIONS_MANUAL_TIME вернется в автоматический режим).
            P_RESTRICTIONS_MODE,

            ///< Работа модуля ограничений в ручном режиме заданное время.
            P_RESTRICTIONS_MANUAL_TIME,

            ///< Переход на паузу операции при ошибке устройств,
            /// 0 - авто (есть), 1 - ручной (нет).
            P_AUTO_PAUSE_OPER_ON_DEV_ERR,
            };

        saved_params_u_int_4 par;

        int set_cmd( const char *prop, unsigned int idx, double val );
        bool is_emulator();
    };
//----------------------------------------------------------------------------
class siren_lights_manager
    {
    public:
        int init( device *red, device *yellow, device *green, device *srn );

        /// @brief Задание типа мигания для красной лампочки.
        ///
        /// @param type - 0 - реализуем сами, 1 - встроенный в сирену.
        void set_red_blink( int type );
    };
//-----------------------------------------------------------------------------
siren_lights_manager* G_SIREN_LIGHTS_MANAGER();
//----------------------------------------------------------------------------
PAC_info* G_PAC_INFO();
//-----------------------------------------------------------------------------
/// @brief Получение текущего времени в секундах.
///
/// @return Текущее время.
/// @warning Время возвращается в секундах с 01.01.1970, в 2038 произойдет
/// переполнение.
unsigned long get_sec();
//-----------------------------------------------------------------------------
/// @brief Получение времени в миллисекундах.
///
/// Так как за 4 дня происходит переполнение и отсчет продолжается с 0, то для
/// вычисления разности времени использовать @ref get_delta_millisec.
///
/// @return Время с момента запуска программы в миллисекундах.
unsigned long get_millisec();
//-----------------------------------------------------------------------------
/// @brief Получение разности времени в миллисекундах.
///
/// @param time1     - начальное время.
/// @return Разность времени в миллисекундах.
unsigned long get_delta_millisec( unsigned long time1 );
//-----------------------------------------------------------------------------
/// @brief Ожидание заданное время.
///
/// @param ms - время ожидания, мс.
void sleep_ms( unsigned long ms );
//-----------------------------------------------------------------------------
/// @brief Структура, описывающая текущую дату и время.
struct tm {
    int tm_sec;     /// Seconds after the minute - [0,59].
    int tm_min;     /// Minutes after the hour - [0,59].
    int tm_hour;    /// Hours since midnight - [0,23].
    int tm_mday;    /// Day of the month - [1,31].
    int tm_mon;     /// Months since January - [0,11].
    int tm_year;    /// Years since 1900.
    int tm_wday;    /// Days since Sunday - [0,6].
    int tm_yday;    /// Days since January 1 - [0,365].
    int tm_isdst;   /// Daylight savings time flag.
    };
//-----------------------------------------------------------------------------
/// @brief Получение текущей информации о времени.
///
/// @return Текущая дата и время.
tm get_time();
////-----------------------------------------------------------------------------
/// @brief Класс регулятора для моечной станции
class MSAPID
    {
    public:
        void eval();
        void eval(float inputvalue, float task);
        void reset();
        void on( int accel = 0 );
        void off();
        void set( float new_z );
        int get_state();
    };
//
/// @brief Класс для модуля моечной станции
class cipline_tech_object: public tech_object
    {
    public:
        cipline_tech_object( const char* name, unsigned int number, unsigned int type, const char* name_Lua,
            unsigned int states_count,
            unsigned int timers_count,
            unsigned int par_float_count, unsigned int runtime_par_float_count,
            unsigned int par_uint_count, unsigned int runtime_par_uint_count );

        int blocked;
        int opcip;
        int curstep;
        int state;
        int curprg;
        int loadedRecipe;
        int loadedProgram;
        int nmr;

        bool nplaststate;   //Последнее состояние подающего насоса.
        bool pidf_override; //Управление пид-регулятором насоса подачи из скрипта.
        int cip_in_error;
        char no_neutro; ///Флаг отсутствия нейтрализации
        char dont_use_water_tank; //Флаг возможности использования танка вторичной воды для мойки
        int disable_tank_heating; //отключение подогрева при начале подачи растворов в танк(для МСА со старыми регулирующими клапанами)
        int ret_overrride; //флаг принудительного включения/выключения возвратного насооса
        int return_ok; //есть расход на возврате
        int concentration_ok; //есть концентрация на возврате
        int enable_ret_pump; //используется для того, чтобы определить, нужно ли отключать возвратный насос
        int clean_water_rinsing_return; //Куда возвращать на операции окончательного ополаскивания
        int scoldvalves; ///Старая логика управления клапанами сортировки растворов при самоочистке.
        int no_acid_wash_max; ///Максимальное количество моек щелочью без кислоты.
        bool use_internal_medium_recipes; //Вкл./выкл. использование рецептов для моющих средств.

        i_DO_device* V00;
        i_DO_device* V01;
        i_DO_device* V02;
        i_DO_device* V03;
        i_DO_device* V04;
        i_DO_device* V05;
        i_DO_device* V06;
        i_DO_device* V07;
        i_DO_device* V08;
        i_DO_device* V09;
        i_DO_device* V10;
        i_DO_device* V11;
        i_DO_device* V12;
        i_DO_device* V13;

        i_DO_AO_device* NP;
        i_DO_AO_device* NK;
        i_DO_AO_device* NS;
        i_DI_device* LL;
        i_DI_device* LM;
        i_DI_device* LH;
        i_DI_device* LWL;
        i_DI_device* LWH;
        i_DI_device* LSL;
        i_DI_device* LSH;
        i_DI_device* LKL;
        i_DI_device* LKH;
        i_AI_device* TP;
        i_AI_device* TR;
        i_AI_device* Q;
        i_AO_device* ao;
        i_AO_device* PUMPFREQ;
        i_DI_device* FL;
        i_counter *cnt;
        timer* T[10];

        MSAPID* PIDP;
        MSAPID* PIDF;

        int msa_number; //Номер станции
        char* ncar1; //номер машины
        char* ncar2; //номер машины
        char* ncar3; //номер машины
        char* ncar4; //номер машины
        int switch1;
        int switch2;
        int switch3;
        int switch4;
        saved_params_float      par_float;   ///< Сохраняемые пар-ры, тип float.
        run_time_params_float   rt_par_float;///< Рабочие параметры, тип float.
        float get_station_par(int parno);
        void set_station_par(int parno, float newval);
        float get_selfclean_par(int parno);
        void set_selfclean_par(int parno, float newval);

        int set_cmd( const char *prop, unsigned int idx, const char* val );
        int set_cmd( const char *prop, unsigned int idx, double val );

        void initline();
        int evaluate();

        float GetConc( int what );
        void SortRR( int where, int forcetotank);
        int SetRet(int val);
        int ForceRet(int val);
        int GetRetState();
        int HasRet();
        int timeIsOut();

        //Базовые методы для вызова из модифицированных на LUA
        virtual int _DoStep(int step_to_do);
        virtual int _GoToStep(int cur, int param);
        virtual int _InitStep(int step_to_init, int not_first_call);
        virtual int _LoadProgram(void);
        virtual void _StopDev(void);
        virtual void _ResetLinesDevicesBeforeReset(void);
        virtual int _OporCIP(int where);
        virtual int _InitOporCIP(int where, int step_to_init, int not_first_call);
        virtual int _CheckErr(void);
        virtual int _Circ(int what);
        virtual int _InitCirc(int what, int step_to_init, int not_first_call);
        virtual int _InitToObject(int from, int where, int step_to_init, int f);
        virtual int _InitFromObject(int what, int where, int step_to_init, int f);
        virtual int _InitFilCirc(int with_what, int step_to_init, int f);
        virtual int _InitOporCirc(int where, int step_to_init, int not_first_call);
        virtual int _ToObject(int from, int where);
        virtual int _FromObject(int what, int where);
        virtual int _FillCirc(int with_what);
        virtual int _OporCirc(int where);
        virtual void _RT(void);
        virtual void _Stop(int step_to_stop);
        virtual int _InitDoseRR(int what, int step_to_init, int not_first_call);
        virtual int _DoseRR(int what);
    };
//--------------------------------------------------------------------------
class modbus_client
    {
    protected:

    public:
        modbus_client(unsigned int id, char* ip, unsigned int port, unsigned long exchangetimeout);
        //реализация функций протокола modbus
        int read_discrete_inputs(unsigned int start_address, unsigned int quantity);
        int read_coils(unsigned int start_address, unsigned int quantity);
        int read_holding_registers(unsigned int address, unsigned int quantity);
        int read_input_registers(unsigned int address, unsigned int quantity);
        int write_coil(unsigned int address, unsigned char value);
        int force_multiply_coils(unsigned int address, unsigned int quantity);
        int write_multiply_registers(unsigned int address, unsigned int quantity);
        int async_read_discrete_inputs(unsigned int start_address, unsigned int quantity);
        int async_read_coils(unsigned int start_address, unsigned int quantity);
        int async_read_holding_registers(unsigned int address, unsigned int quantity);
        int async_read_input_registers(unsigned int address, unsigned int quantity);
        int async_write_coil(unsigned int address, unsigned char value);
        int async_force_multiply_coils(unsigned int address, unsigned int quantity);
        int async_write_multiply_registers(unsigned int address, unsigned int quantity);
        int async_read_write_multiply_registers(unsigned int readaddress, unsigned int readquantity, unsigned int wrireaddress, unsigned int writequantity);
        int async_mask_write_register(unsigned int writeaddress, unsigned int andmask, unsigned int ormask);
        int async_mask_write_register(unsigned int writeaddress);
        int get_async_result();
        void set_station(unsigned char new_station_id);
        //функции для работы с буфером из Lua
        void zero_output_buff(int startpos = 13);
        void set_byte(int address, unsigned char value);
        unsigned char get_byte(int address);
        void set_int2(unsigned int address, short value);
        short get_int2(unsigned int address);
        void set_int4(unsigned int address, int value);
        int get_int4(unsigned int address);
        void set_float(unsigned int address, float value);
        float get_float(unsigned int address);
        void set_bit(unsigned int address, int value);
        int get_bit(unsigned int address);
        int reg_get_bit(unsigned int reg, unsigned int offset);
        void reg_set_bit(unsigned int reg, unsigned int offset, int value);
        void mask_reset();
        void mask_set_bit(int pos, int value);
        unsigned char reverse(unsigned char b);
        int swapBits(int x, int p1, int p2, int n);
        int get_id();
    };
//-------------------------------------------------------------------------
class ModbusServ
    {
    public:
        static short int UnpackInt16( unsigned char* buf, int offset );
        static long int UnpackInt32( unsigned char* buf, int offset );
        static float UnpackFloat( unsigned char* Buf, int offset  );
        static unsigned int UnpackWord( unsigned char* Buf, int offset );
    };
//----------------------------------------------------------------------------
/// @brief Работа с Profibus Slave.
class profibus_slave
    {
    //Конфигурирование клиента.
    public:
        /// <summary>
        /// Включение модуля обмена.
        /// </summary>
        void activate();

        /// <summary>
        /// Установка адреса станции.
        /// </summary>
        void set_station_address( int address );

        /// <summary>
        /// Установка размера массива области записи.
        /// </summary>
        void set_output_byte_size( int size );

        /// <summary>
        /// Установка размера массива области чтения.
        /// </summary>
        void set_input_byte_size( int size );

    public:
        /// <summary>
        /// Получение значения типа double.
        /// </summary>
        /// <param name="offset">Смещение, диапазон 0..239.</param>
        virtual double get_double( int offset ) = 0;

        /// <summary>
        /// Получение значения типа bool.
        /// </summary>
        /// <param name="byte_offset">Смещение, диапазон 0..243.</param>
        /// <param name="bit_offset">Смещение, диапазон 0..7.</param>
        virtual bool get_bool( int byte_offset, int bit_offset ) = 0;

        /// <summary>
        /// Установка значения типа bool.
        /// </summary>
        /// <param name="byte_offset">Смещение, диапазон 0..243.</param>
        /// <param name="bit_offset">Смещение, диапазон 0..7.</param>
        /// <param name="val">Значение.</param>
        virtual void set_bool( int byte_offset, int bit_offset, bool val ) = 0;

        /// <summary>
        /// Получение значения типа int.
        /// </summary>
        /// <param name="byte_offset">Смещение, диапазон 0..242.</param>
        virtual int get_int( int byte_offset ) = 0;

        /// <summary>
        /// Установка значения типа int.
        /// </summary>
        /// <param name="byte_offset">Смещение, диапазон 0..242.</param>
        /// <param name="val">Значение.</param>
        virtual void set_int( int byte_offset, int val ) = 0;

        /// <summary>
        /// Получение значения типа int (4 байта).
        /// </summary>
        /// <param name="byte_offset">Смещение, диапазон 0..240.</param>
        virtual int get_int4( int byte_offset ) = 0;
    };

profibus_slave* G_PROFIBUS_SLAVE_LUA();
//-----------------------------------------------------------------------------
class i_log
    {
    enum PRIORITIES
        {
        P_EMERG, 	// System is unusable
        P_ALERT,	// Action must be taken immediately
        P_CRIT,		// Critical conditions
        P_ERR,		// Error conditions
        P_WARNING,	// Warning conditions
        P_NOTICE,	// Normal but significant condition
        P_INFO,		// Informational
        P_DEBUG,	// Debug-level messages
        };

    /// @brief Запись строки в журнал.
    ///
    /// @param priority - приоритет.
    /// @debug_message - строка.
    void write_log( PRIORITIES priority, const char* debug_message );
    };

i_log* G_SYS_LOG();
//-----------------------------------------------------------------------------
