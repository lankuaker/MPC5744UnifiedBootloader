/* ###################################################################
**     This component module is generated by Processor Expert. Do not modify it.
**     Filename    : eTimer1.h
**     Project     : MPC5744_bootloader
**     Processor   : MPC5744P_257
**     Component   : etimer
**     Version     : Component SDK_S32_PA_11, Driver 01.00, CPU db: 3.00.000
**     Repository  : SDK_S32_PA_11
**     Compiler    : GNU C Compiler
**     Date/Time   : 2022-09-22, 10:32, # CodeGen: 8
**     Contents    :
**         ETIMER_DRV_Init                                            - void ETIMER_DRV_Init(const uint16_t instance);
**         ETIMER_DRV_Deinit                                          - void ETIMER_DRV_Deinit(const uint16_t instance);
**         ETIMER_DRV_GetDefaultChannelConfig                         - void ETIMER_DRV_GetDefaultChannelConfig(etimer_user_channel_config_t * config);
**         ETIMER_DRV_GetDefaultChannelConfigOneshot                  - void ETIMER_DRV_GetDefaultChannelConfigOneshot(etimer_user_channel_config_t *...
**         ETIMER_DRV_GetDefaultChannelConfigPulseOutput              - void ETIMER_DRV_GetDefaultChannelConfig...
**         ETIMER_DRV_GetDefaultChannelConfigFixedFreqPwm             - void ETIMER_DRV_GetDefaultChannelConfig...
**         ETIMER_DRV_GetDefaultChannelConfigVariableFreqPwm          - void ETIMER_DRV_GetDefaultChannelConfig...
**         ETIMER_DRV_GetDefaultChannelConfigModuleCountingDirection  - void ETIMER_DRV_GetDefaultChannelConfig...
**         ETIMER_DRV_GetDefaultChannelConfigModuleCountingQuadrature - void ETIMER_DRV_GetDefaultChannelConfig...
**         ETIMER_DRV_GetDefaultDmaChannel                            - void ETIMER_DRV_GetDefaultDmaChannel(etimer_dma_channel_t * config);
**         ETIMER_DRV_InitChannel                                     - void ETIMER_DRV_InitChannel(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_DmaInitRequest                                  - void ETIMER_DRV_DmaInitRequest(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_ChannelDebugBehaviour                           - void ETIMER_DRV_ChannelDebugBehaviour(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_StartTimerChannels                              - void ETIMER_DRV_StartTimerChannels(const uint16_t instance,const uint16_t mask);
**         ETIMER_DRV_StopTimerChannels                               - void ETIMER_DRV_StopTimerChannels(const uint16_t instance,const uint16_t mask);
**         ETIMER_DRV_IsTimerRunning                                  - bool ETIMER_DRV_IsTimerRunning(const uint16_t instance,const uint16_t channel);
**         ETIMER_DRV_SetTimerTicks                                   - void ETIMER_DRV_SetTimerTicks(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_GetTimerTicks                                   - uint16_t ETIMER_DRV_GetTimerTicks(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_SetTimerTicksBuffered                           - void ETIMER_DRV_SetTimerTicksBuffered(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_EnableInterruptSource                           - void ETIMER_DRV_EnableInterruptSource(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_DisableInterruptSource                          - void ETIMER_DRV_DisableInterruptSource(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_EnableDisableInterruptSources                   - void ETIMER_DRV_EnableDisableInterruptSources(const uint16_t instance,const...
**         ETIMER_DRV_GetInterruptStatus                              - uint16_t ETIMER_DRV_GetInterruptStatus(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_ClearInterruptStatus                            - void ETIMER_DRV_ClearInterruptStatus(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_GetInterruptNumber                              - IRQn_Type ETIMER_DRV_GetInterruptNumber(const uint16_t instance,const...
**         ETIMER_DRV_SetCompareThreshold                             - void ETIMER_DRV_SetCompareThreshold(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_SetCompareThresholdBuffered                     - void ETIMER_DRV_SetCompareThresholdBuffered(const uint16_t instance,const...
**         ETIMER_DRV_OutputPinEnable                                 - void ETIMER_DRV_OutputPinEnable(const uint16_t instance,const uint16_t channel);
**         ETIMER_DRV_OutputPinDisable                                - void ETIMER_DRV_OutputPinDisable(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_StartCapture                                    - void ETIMER_DRV_StartCapture(const uint16_t instance,const uint16_t channel);
**         ETIMER_DRV_StopCapture                                     - void ETIMER_DRV_StopCapture(const uint16_t instance,const uint16_t channel);
**         ETIMER_DRV_GetCaptureValue                                 - uint16_t ETIMER_DRV_GetCaptureValue(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_GetCaptureConfigWords                           - uint16_t ETIMER_DRV_GetCaptureConfigWords(const uint16_t instance,const...
**         ETIMER_DRV_GetCaptureWords                                 - uint16_t ETIMER_DRV_GetCaptureWords(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_SetWatchdog                                     - void ETIMER_DRV_SetWatchdog(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_DisableWatchdog                                 - void ETIMER_DRV_DisableWatchdog(const uint16_t instance,const uint16_t channel);
**         ETIMER_DRV_ForceOutputLogicLevel                           - void ETIMER_DRV_ForceOutputLogicLevel(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_GetCompareThresholdBuffered                     - void ETIMER_DRV_GetCompareThresholdBuffered(const uint16_t instance,const...
**         ETIMER_DRV_ReloadOnCompare                                 - void ETIMER_DRV_ReloadOnCompare(const uint16_t instance,const uint16_t...
**         ETIMER_DRV_SetOutputFunction                               - void ETIMER_DRV_SetOutputFunction(const uint16_t instance,const uint16_t...
**
**     Copyright 1997 - 2015 Freescale Semiconductor, Inc. 
**     Copyright 2016-2017 NXP 
**     All Rights Reserved.
**     
**     THIS SOFTWARE IS PROVIDED BY NXP "AS IS" AND ANY EXPRESSED OR
**     IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
**     OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
**     IN NO EVENT SHALL NXP OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
**     INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
**     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
**     SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
**     HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
**     STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
**     IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
**     THE POSSIBILITY OF SUCH DAMAGE.
** ###################################################################*/
/*!
** @file eTimer1.h
** @version 01.00
*/         
/*!
**  @addtogroup eTimer1_module eTimer1 module documentation
**  @{
*/         
#ifndef eTimer1_H
#define eTimer1_H
/* MODULE eTimer1. */

/**
 * @page misra_violations MISRA-C:2012 violations
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 2.5, global macro not referenced
 * There are global macros defined to be used by the integrator and another one used as include guard.
 *
 * @section [global]
 * Violates MISRA 2012 Advisory Rule 8.7, External could be made static.
 * Function is defined for usage by application code.
 */

/* Include inherited beans */
#include "Cpu.h"
#include "etimer_driver.h"

/*! @brief Device instance number */
#define INST_ETIMER1 0U

/*! @brief etimer_user_channel_config_t 0 declaration */
extern const etimer_user_channel_config_t eTmr0Ch3Cfg;
/*! @brief etimer_dma_channel_t declaration */
extern const etimer_dma_channel_t eTimer1_etimer_dma_channel0;
    
#endif
/* ifndef eTimer1_H */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.1 [05.21]
**     for the NXP C55 series of microcontrollers.
**
** ###################################################################
*/