//******** STM.C.1.1 ********//

// ▼ CubeMX 需配置（在STM32CubeMX 中打开.ioc工程配置文件）：
// ① 时钟源：左侧 Pinout & Configuration → Categories → System Core → RCC → High Speed Clock (HSE) → 选 “Crystal/Ceramic Resonator”
// ② 调试口：Categories → System Core → SYS → Debug → 选 “Serial Wire”（否则会禁用 SWD，导致之后无法烧录，如果烧录了禁用SWD的版本，需要将boot0连接到1 然后烧录新版本进行覆盖）
// ③ 时钟树：上方 Clock Configuration 标签页 → Input frequency 填 8（MHz，外部晶振）
//    → PLL Source 选 HSE → PLL Mul 选 ×9 → System Clock Mux 选 PLLCLK（HCLK 自动显示 72MHz）
//    → APB1 Prescaler 选 /2（36MHz，APB1 上限 36MHz）→ APB2 Prescaler 选 /1（72MHz）
//    （72MHz 是最常用的主频；TIM2/TIM3 因此在 72MHz 下计数）
// ④ 引脚：PA0~PA4 不需要在 CubeMX 里配，本文件的 LED_Init() 会自己使能 GPIOA 时钟并配成推挽输出
//    ★ 接线是高电平点亮：PAx → 220Ω 电阻 → LED 长脚(+)，LED 短脚(-) → GND，5 个灯共地
// ⑤ 点右上角 GENERATE CODE；main() 会自动调用 SystemClock_Config() 与 MX_GPIO_Init()
//    （CubeMX 不会重新生成 chapter.c，本文件代码不会被覆盖）

#include "main.h"      /* HAL 类型与 GPIO_PIN_x 引脚编号都来自它 */

/*
- 宏定义：把“几个灯”“多久换一次”这类参数集中写在前面，改需求时只改这里。
*/
#define LED_NUM          5U         /* LED 个数 */
#define LED_INTERVAL_MS  5000U      /* 每个状态持续时间（毫秒）；想快就改成 1000 / 500 */

/*
- 极性开关：决定“写什么电平算点亮”。
  LED_ACTIVE_LOW = 0 → 高电平点亮：PAx ─电阻─ LED长脚，LED短脚 ─ GND
  LED_ACTIVE_LOW = 1 → 低电平点亮：3.3V ─ LED长脚，LED短脚 ─电阻─ PAx
  如果上电自检（5 个灯同时快闪 3 次）一个都不亮，先试着把这里改成 1，重新编译烧录。
*/
#define LED_ACTIVE_LOW   0

/*
- 上电自检参数：程序一启动先让 5 个灯同时“亮 1 秒 / 灭 1 秒”重复 3 次，
  用来单独验证“引脚 → 电阻 → LED → GND”这条链路。自检通过后再进入流水灯。
*/
#define SELFTEST_TIMES   3U
#define SELFTEST_MS      1000U

/*
- 查表：端口和引脚分开存成数组，循环里按下标访问，比写 5 个 if 清爽。
  以后想把灯换到别的引脚，只改下面两行即可。
*/
static GPIO_TypeDef *const LED_PORT[LED_NUM] = { GPIOA, GPIOA, GPIOA, GPIOA, GPIOA };
static const uint16_t      LED_PIN [LED_NUM] = { GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4 };

/* 点亮 / 熄灭对应的电平，由 LED_ACTIVE_LOW 决定 */
static GPIO_PinState LED_OnLevel(void)
{
    return (LED_ACTIVE_LOW != 0) ? GPIO_PIN_RESET : GPIO_PIN_SET;
}

static GPIO_PinState LED_OffLevel(void)
{
    return (LED_ACTIVE_LOW != 0) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/*
- 统一操作 5 个灯：level 传 LED_OnLevel() 就是全亮，传 LED_OffLevel() 就是全灭。
*/
static void LED_SetAll(GPIO_PinState level)
{
    for (uint32_t i = 0; i < LED_NUM; i++)
    {
        HAL_GPIO_WritePin(LED_PORT[i], LED_PIN[i], level);
    }
}

/*
- 引脚初始化：main() 里的 MX_GPIO_Init() 只配置了 PC13，PA0~PA4 仍是默认的“浮空输入”，
  这时写电平不会有任何反应（现象就是五个灯全不亮），所以这里自己补一次初始化。
  （即使 CubeMX 里也把 PA0~PA4 配成了输出，重复配置一次也无害。）
*/
static void LED_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();          /* 使能 GPIOA 时钟，不使能则寄存器写不进去 */

    LED_SetAll(LED_OffLevel());            /* 先把 ODR 写好，避免初始化瞬间闪一下 */

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;    /* 推挽输出 */
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;    /* 点亮 LED 不需要高速 */
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/*
- 上电自检：5 个灯同时亮 1 秒、灭 1 秒，重复 3 次。
  亮 → 接线正确；不亮 → 查接地 / 电阻 / LED 方向，或把 LED_ACTIVE_LOW 改成 1。
*/
static void LED_SelfTest(void)
{
    for (uint32_t n = 0; n < SELFTEST_TIMES; n++)
    {
        LED_SetAll(LED_OnLevel());
        HAL_Delay(SELFTEST_MS);
        LED_SetAll(LED_OffLevel());
        HAL_Delay(SELFTEST_MS);
    }
}

/*
- 函数调用：通过调用函数来执行特定的任务， main() 已调用 chapter_main() 来执行我们的内容。
*/
void chapter_main(void)
{
    LED_Init();                                            /* 把 PA0~PA4 配成推挽输出 */

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   /* 顺手关掉板载 LED，免得干扰观察 */

    LED_SelfTest();                                        /* 先自检接线，再开始流水 */

    uint32_t i = 0;    /* 当前要点亮第几个灯（0 ~ LED_NUM-1） */

    /*
    - while 循环：无限循环，程序将一直在此运行。若想退出循环并向下运行，需要 break;。
    */
    while (1)
    {
        LED_SetAll(LED_OffLevel());                                   /* 全部熄灭 */
        HAL_GPIO_WritePin(LED_PORT[i], LED_PIN[i], LED_OnLevel());     /* 只点亮第 i 个 */
        HAL_Delay(LED_INTERVAL_MS);                                   /* 保持一段时间 */

        i++;                       /* 轮到下一个灯 */
        if (i >= LED_NUM)          /* 走到最后一个就回到第一个，形成“流水” */
        {
            i = 0;
        }
    }
}