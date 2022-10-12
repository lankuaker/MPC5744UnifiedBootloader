#include "fls_app.h"
#include "CRC_hal.h"
#include "watchdog_hal.h"
#include "uds_app.h"


#define MAX_DATA_ITEM (16u)      /*max data item*/
#define MAX_DATA_BUF_LEN (128u)  /*max data buf len*/

/*For resolved the issue when undefine MAX_ONCE_CHECKSUM_SIZE*/
#ifndef MAX_ONCE_CHECKSUM_SIZE
/*default max once checksum size byte*/
#define MAX_ONCE_CHECKSUM_SIZE (100000u)
#endif

typedef struct
{
	uint32 startAddr;   /*start addr*/
	uint32 dataLen;     /*data len*/
}tFlashDataInfo;

typedef struct
{
	uint8 numberOfItem; /*number of item*/
	tFlashDataInfo astFlashDataItem[MAX_DATA_ITEM]; /*item flash data*/
	tCrc xReceivedCrc;
}tFlashDataCheckInfo;

typedef struct
{
	/*flash programming successfull? If programming successfull, the value set TRUE, else set FALSE*/
	uint8 isFlashProgramSuccessfull;  

	/*Is erase flash successfull? If erased flash successfull, set the TRUE, else set the FALSE.*/
	uint8 isFlashErasedSuccessfull;   

	/*Is Flash struct data valid? If writen set the value is TRUE, else set the valid FALSE*/
	uint8 isFlashStructValid;

	/*indicate app Counter. Before download. */
	uint8 appCnt; 

	/* flag if fingerprint buffer */
    uint8 aFingerPrint[FL_FINGER_PRINT_LENGTH];

	/*reset handler length*/
	uint32 appStartAddrLen;

	/*app Start address -- reset handler*/
	uint32 appStartAddr;

	/*count CRC*/
	uint32 crc;
}tAppFlashStatus;


typedef struct
{
    /* flag if fingerprint has written */
    uint8 isFingerPrintWritten;
    
    /* flag if flash driver has downloaded */
    uint8 isFlashDrvDownloaded;
    
    /* error code for flash active job */
    uint8 errorCode;

	/*erase app type flag*/
	uint8 erasedAPPFlag;
	
	/*request active job UDS service ID*/
	uint8 requestActiveJobUDSSerID;

	/*storage program data buff*/
    uint8 aProgramDataBuff[MAX_FLASH_DATA_LEN];
	
	/* current procees start address */
    uint32 startAddr;
    
    /* current procees length */
    uint32 length;

	/*recieve data start address*/
	uint32 receivedDataStartAddr;

	/*received data length*/
	uint32 receivedDataLength;

	/*received CRC value*/
	uint32 receivedCRC;

	/*calculate CRC value*/
	uint32 calculateCRCValue;

	/*received program data length*/
	uint32 receiveProgramDataLength;
	        
    /* flashloader download step */
    tFlDownloadStepType eDownloadStep;
    
    /* current job status */
    tFlshJobModle eActiveJob;

	/*interrupted job*/
	tFlshJobModle eInterruptedJob;

	/*active job finshed callback*/
	tpfResponse pfActiveJobFinshedCallBack;

	/*request more time from host*/
	tpfReuestMoreTime pfRequestMoreTime;

	/*point app flash status*/
	tAppFlashStatus *pstAppFlashStatus;

    /*opeate flash API*/
	tFlashOperateAPI stFlashOperateAPI;
	
}tFlsDownloadStateType;

/*Erase flash status control*/
typedef enum
{
	START_ERASE_FLASH,     /*start erase flash*/
	DO_ERASING_FLASH,   /*Do erase flash*/
	END_ERASE_FLASH  /*end erase flash*/
}tEraseFlashStep;

/*Checksum Step*/
typedef enum
{
	START_CHECKSUM = 0u, /*start checksum*/
	DO_CHECKING_CHECKSUM,/*do checking checksum*/
	END_CHECKSUM,        /*end checksum*/
}tChecksumStep;

/*Is flash driver download?*/
#define Flash_IsFlashDriverDownload() (gs_stFlashDownloadInfo.isFlashDrvDownloaded)
#define Flash_SetFlashDriverDowload() (gs_stFlashDownloadInfo.isFlashDrvDownloaded = TRUE)
#define Flash_SetFlashDriverNotDonwload() (gs_stFlashDownloadInfo.isFlashDrvDownloaded = FALSE)

/*flash download info*/
static tFlsDownloadStateType gs_stFlashDownloadInfo;

/*erase flash status.*/
static tEraseFlashStep gs_eEraseFlashStep = START_ERASE_FLASH;

/*checksum step*/
static tChecksumStep gs_eChecksumStep = START_CHECKSUM;

/*get current erase falsh step*/
#define Flash_GetCurEraseFlashStep() (gs_eEraseFlashStep)

/*set erase flash status*/
#define Flash_SetEraseFlashStep(eEraseStep) \
do{\
	gs_eEraseFlashStep = (eEraseStep);\
}while(0u)

/*request time status define*/
#define REQ_TIME_SUCCESSFUL (1u)

/*request time failed*/
#define REQ_TIME_FAILED (2u)

/*Is request time successful?*/
static uint8 gs_reqTimeStatus = 0xFFu;

/*application flash status*/
static tAppFlashStatus gs_stAppFlashStatus; 

/*set request time status*/
#define Flash_ClearRequestTimeStauts()\
do{\
	gs_reqTimeStatus = 0xFFu;\
}while(0u)

/*request more time successful*/
#define Flash_SetRequestMoreTimeStatus(status) \
do{\
	gs_reqTimeStatus = status;\
}while(0u)

/*Is request time successfull?*/
#define Flash_IsReqestTimeSuccessfull() ((1u == gs_reqTimeStatus) ? TRUE : FALSE)

#define Flash_IsRequestTimeFailed() (((2u == gs_reqTimeStatus)) ? TRUE : FALSE)

/*Application flash status*/
static boolean Flash_SaveAppInfoData(const uint32 i_appInfoDataStartAddr);

#define Flash_IsFlashAppCrcEqualStorage(xCrc) ((gs_stAppFlashStatus.crc == xCrc) ? TRUE : FALSE)

#define Flash_SetFlashEreaseStatus(bIsFlashEraseSuccessful) \
do{\
	gs_stAppFlashStatus.isFlashErasedSuccessfull = bIsFlashEraseSuccessful;\
}while(0u)

#define Flash_SetFlashProgramStatus(bIsFlashProgramSuccessful) \
do{\
	gs_stAppFlashStatus.isFlashProgramSuccessfull = bIsFlashProgramSuccessful;\
}while(0u)


#define Flash_SetFlashStructStatus(bIsAppFlashStructValid) \
do{\
	gs_stAppFlashStatus.isFlashStructValid = bIsAppFlashStructValid;\
}while(0u)


#define Flash_SetAPPStatus(bIsFlashEraseSuccessful, bIsFlashProgramSuccessful, bIsAppFlashStructValid) \
do{\
	Flash_SetFlashEreaseStatus(bIsFlashEraseSuccessful);\
	Flash_SetFlashProgramStatus(bIsFlashProgramSuccessful);\
	Flash_SetFlashStructStatus(bIsAppFlashStructValid);\
}while(0u)

#define Flash_IsFlashEraseSuccessful() (gs_stAppFlashStatus.isFlashErasedSuccessfull)
#define Flash_IsFlashProgramSuccessful() (gs_stAppFlashStatus.isFlashProgramSuccessfull)
#define Flash_IsFlashStructValid() (gs_stAppFlashStatus.isFlashStructValid)

#define Flash_CreateAppStatusCrc(o_pCrc) do{\
	CRC_HAL_CalculateCRCOnce((uint8 *)&gs_stAppFlashStatus, sizeof(gs_stAppFlashStatus) - 4u, o_pCrc);\
}while(0u)

#define Flash_SaveAppStatusCrc(xCrc)\
do{\
	gs_stAppFlashStatus.crc = xCrc;\
}while(0u)

/*create app status CRC and save*/
#define Flash_CreateAndSaveAppStatusCrc(o_pCrc) \
do{\
	Flash_CreateAppStatusCrc(o_pCrc);\
	Flash_SaveAppStatusCrc(*o_pCrc);\
}while(0u)

#define Flash_GetAppStatusPtr() (&gs_stAppFlashStatus)

#define Flash_SaveAppResetHandlerAddr(resetHandlerAddr, resetHnadlerAddrLen) \
do{\
	gs_stAppFlashStatus.appStartAddr = resetHandlerAddr;\
	gs_stAppFlashStatus.appStartAddrLen = resetHnadlerAddrLen;\
}while(0u)

/*Is flash driver software data?*/
static boolean Flash_IsFlashDriverSoftwareData(void);

/*flash write */
static uint8 Flash_Write(boolean *o_pbIsOperateFinsh);

/*flash check sum */
static uint8 Flash_Checksum(boolean *o_pbIsOperateFinsh);

/*do checksum*/
static boolean Flash_DoChecksum(boolean *o_pbIsOperateFinsh);

/*Is received CRC valid?*/
static boolean Flash_IsReceivedCRCValid(void);

/*save calcualte CRC value*/
static void Flash_SaveCalculateCRCValue(const uint32 i_CRCValue);

/* Fash Erase*/
static uint8 Flash_Erase(boolean * o_pbIsOperateFinsh);

/*save flash data buf*/
static uint8 Flash_SavedFlashData(const uint8 *i_pDataBuf, const uint8 i_dataLen);

/*read application informaiton from flash*/
static void Flash_ReadNewestAppInfoFromFlash(void);

/*restore operate flash active job*/
static void Flash_RestoreOperateFlashActiveJob(const tFlshJobModle i_activeJob);

/*register interrupt job*/
static void Flash_RegInterruptedJob(const tFlshJobModle i_activeJob);

/*restore interrupted job*/
static void Flash_RestoreInterruptedJob(void);

#ifdef EN_SUPPORT_APP_B
static tAPPType Flash_DoCheckNewestAPPCnt(const tAppFlashStatus * i_pAppAInfo, const tAppFlashStatus *i_pAppBInfo);

/*Do check newest app information*/
static tAPPType Flash_DoCheckNewestAPPInfo(const tAppFlashStatus * i_pAppAInfo, const tAppFlashStatus *i_pAppBInfo);
#endif

/*Set app type ersed*/
static void Flash_SetAPPTypeErased(const tAPPType i_erasedAPPType);

/*clear app type erased status*/
static void Flash_ClearAPPTypeErased(const tAPPType i_erasedAPPType);

/*Init flash download*/
void Flash_InitDowloadInfo(void)
{	
	gs_stFlashDownloadInfo.isFingerPrintWritten = FALSE;

	if(TRUE == Flash_IsFlashDriverDownload())
	{
		Flash_EraseFlashDriverInRAM();

		Flash_SetFlashDriverNotDonwload();
	}	

	Flash_SetNextDownloadStep(FL_REQUEST_STEP);

	Flash_SetOperateFlashActiveJob(FLASH_IDLE, NULL_PTR, INVALID_UDS_SERVICES_ID, NULL_PTR);

	gs_stFlashDownloadInfo.pstAppFlashStatus = &gs_stAppFlashStatus;
	
	fsl_memset(&gs_stFlashDownloadInfo.stFlashOperateAPI, 0x0u, sizeof(tFlashOperateAPI));

	fsl_memset(&gs_stAppFlashStatus, 0xFFu, sizeof(tAppFlashStatus));
}

/*flash app module init*/
void FLASH_APP_Init(void)
{
	gs_stFlashDownloadInfo.isFingerPrintWritten = FALSE;

	Flash_EraseFlashDriverInRAM();

	Flash_SetFlashDriverNotDonwload();

	Flash_SetNextDownloadStep(FL_REQUEST_STEP);

	Flash_SetOperateFlashActiveJob(FLASH_IDLE, NULL_PTR, INVALID_UDS_SERVICES_ID, NULL_PTR);

	gs_stFlashDownloadInfo.pstAppFlashStatus = &gs_stAppFlashStatus;
	
	fsl_memset(&gs_stFlashDownloadInfo.stFlashOperateAPI, 0x0u, sizeof(tFlashOperateAPI));

	fsl_memset(&gs_stAppFlashStatus, 0xFFu, sizeof(tAppFlashStatus));	
}

/*flash operate main function*/
void Flash_OperateMainFunction(void)
{
	tFlshJobModle currentFlashJob = FLASH_IDLE;
	boolean bIsOperateFinshed = FALSE;
	
	currentFlashJob = Flash_GetOperateFlashActiveJob();
	
    switch (currentFlashJob)
    {
        case FLASH_ERASING:
			
            /* do the flash erase*/
			bIsOperateFinshed = FALSE;
            gs_stFlashDownloadInfo.errorCode = Flash_Erase(&bIsOperateFinshed);
			
            break;

        case FLASH_PROGRAMMING:	
			
			bIsOperateFinshed = TRUE;
			
            /* do the flash program*/
            gs_stFlashDownloadInfo.errorCode = Flash_Write(&bIsOperateFinshed);
		
            break;

        case FLASH_CHECKING:
			
            /* do the flash checksum*/
			bIsOperateFinshed = TRUE;
            gs_stFlashDownloadInfo.errorCode = Flash_Checksum(&bIsOperateFinshed);
		
            break;

		case FLASH_WAITTING:

			if(TRUE == Flash_IsReqestTimeSuccessfull())
			{
				Flash_ClearRequestTimeStauts();

				Flash_RestoreInterruptedJob();
				//Flash_RestoreOperateFlashActiveJob(FLASH_ERASING);
			}
			else if(TRUE == Flash_IsRequestTimeFailed())
			{			
				Flash_ClearRequestTimeStauts();

				/*set erase flash step to start!*/
				Flash_SetEraseFlashStep(START_ERASE_FLASH);

		        /* initialize the flash download state */
		        Flash_InitDowloadInfo();				
				
				/*set flash job is IDLE*/
				Flash_SetOperateFlashActiveJob(FLASH_IDLE, NULL_PTR, INVALID_UDS_SERVICES_ID, NULL_PTR);
			}
			else
			{
				/*do nothing*/
				
			}

			break;

        default:
            break;
    }

	/*just operate flash finshed, can do callback and set next job.*/
	if(TRUE == bIsOperateFinshed)
	{
        if((NULL_PTR != gs_stFlashDownloadInfo.pfActiveJobFinshedCallBack) && (FLASH_IDLE != currentFlashJob))
        {
            (gs_stFlashDownloadInfo.pfActiveJobFinshedCallBack)(gs_stFlashDownloadInfo.errorCode);

            gs_stFlashDownloadInfo.pfActiveJobFinshedCallBack = NULL_PTR;
        }

	    if ((gs_stFlashDownloadInfo.errorCode != TRUE) && 
			((FLASH_ERASING == currentFlashJob) || 
			 (FLASH_PROGRAMMING == currentFlashJob) ||
			 (FLASH_CHECKING == currentFlashJob)))
	    {
	        /* initialize the flash download state */
	        Flash_InitDowloadInfo();
	    }

		
		/*set flash job is IDLE*/
		Flash_SetOperateFlashActiveJob(FLASH_IDLE, NULL_PTR, INVALID_UDS_SERVICES_ID, NULL_PTR);
	}
}

/*set next downlaod step.*/
void Flash_SetNextDownloadStep(const tFlDownloadStepType i_donwloadStep)
{
	gs_stFlashDownloadInfo.eDownloadStep = i_donwloadStep;
}

/*get current donwload step*/
tFlDownloadStepType Flash_GetCurDownloadStep(void)
{
	return gs_stFlashDownloadInfo.eDownloadStep;
}

/*judaged donwload step*/
boolean Flash_IsEqualDonwloadStep(const tFlDownloadStepType i_donwloadStep)
{
	boolean result = FALSE;
	
	if(i_donwloadStep == Flash_GetCurDownloadStep())
	{
		result =  TRUE;
	}

	return result;
}

/*save download data information, the API called by UDS request download service*/
void Flash_SaveDownloadDataInfo(const uint32 i_dataStartAddr, const uint32 i_dataLen)
{
	/*program data info*/
	gs_stFlashDownloadInfo.startAddr = i_dataStartAddr;
	gs_stFlashDownloadInfo.length = i_dataLen;

	/*calculate data CRC info*/
	gs_stFlashDownloadInfo.receivedDataStartAddr = i_dataStartAddr;
	gs_stFlashDownloadInfo.receivedDataLength = i_dataLen;
}

/*set operate flash active job.*/
void Flash_SetOperateFlashActiveJob(const tFlshJobModle i_activeJob, 
									const tpfResponse i_pfActiveFinshedCallBack, 
									const uint8 i_requestUDSSerID, 
									const tpfReuestMoreTime i_pfRequestMoreTimeCallback)
{
	gs_stFlashDownloadInfo.eActiveJob = i_activeJob;
	gs_stFlashDownloadInfo.requestActiveJobUDSSerID = i_requestUDSSerID;
	gs_stFlashDownloadInfo.pfRequestMoreTime = i_pfRequestMoreTimeCallback;
	gs_stFlashDownloadInfo.pfActiveJobFinshedCallBack = i_pfActiveFinshedCallBack;
}

/*restore operate flash active job*/
static void Flash_RestoreOperateFlashActiveJob(const tFlshJobModle i_activeJob)
{
	gs_stFlashDownloadInfo.eActiveJob = i_activeJob;
}

/*register interrupt job*/
static void Flash_RegInterruptedJob(const tFlshJobModle i_activeJob)
{
	gs_stFlashDownloadInfo.eInterruptedJob = (tFlshJobModle)i_activeJob;
}

/*restore interrupted job*/
static void Flash_RestoreInterruptedJob(void)
{
	gs_stFlashDownloadInfo.eActiveJob = gs_stFlashDownloadInfo.eInterruptedJob;
}

/*get operate flash active job*/
tFlshJobModle Flash_GetOperateFlashActiveJob(void)
{
	return gs_stFlashDownloadInfo.eActiveJob;
}

/*Register flash job callbck. Callback will called when after flash job.*/
void Flash_RegisterJobCallback(tpfResponse i_pfDoResponse)
{
	gs_stFlashDownloadInfo.pfActiveJobFinshedCallBack = i_pfDoResponse;
}

/*save flash data buf*/
static uint8 Flash_SavedFlashData(const uint8 *i_pDataBuf, const uint8 i_dataLen)
{
	ASSERT(NULL_PTR == i_pDataBuf);

	if(i_dataLen > MAX_FLASH_DATA_LEN)
	{
		return FALSE;
	}

	fsl_memcpy(gs_stFlashDownloadInfo.aProgramDataBuff, i_pDataBuf, i_dataLen);

	gs_stFlashDownloadInfo.receiveProgramDataLength = i_dataLen;

	return TRUE;
}

/*Request mo retime sccessful from host. 
**i_txMsgStatus: 0 is successful, others is failed.
*/
static void RequetMoreTimeSuccessfulFromHost(uint8 i_txMsgStatus)
{
	if(0u == i_txMsgStatus)
	{	
		/*request time successful*/
		Flash_SetRequestMoreTimeStatus(REQ_TIME_SUCCESSFUL);
	}
	else
	{
		/*tx message failed.*/
		Flash_SetRequestMoreTimeStatus(REQ_TIME_FAILED);
	}
}

/* Fash Erase*/
static uint8 Flash_Erase(boolean * o_pbIsOperateFinsh)
{
	static uint8 s_result = TRUE;
	uint32 eraseFlashLen = 0u;
	tCrc xCountCrc = 0u;
	static tAPPType s_appType = APP_INVLID_TYPE;
	static BlockInfo_t * s_pAppFlashMemoryInfo = NULL_PTR;
	static uint32 s_appFlashItem = 0u;
	uint32 sectorNo = 0u;
	const uint32 maxEraseSectors = UDS_GetUDSS3WatermarkTimerMs() / FLASH_HAL_GetEraseFlashASectorMaxTimeMs();
	uint32 totalSectors = 0u;
	uint32 canEraseMaxSectors = 0u;
	static uint32 s_eraseSectorsCnt = 0u;
	uint32 eraseFlashStartAddr = 0u;
	uint32 eraseSectorNoTmp = 0u;
	boolean ret = false;

	ASSERT(NULL_PTR == o_pbIsOperateFinsh);

    /*check flash driver valid or not?*/
	if(TRUE != Flash_IsFlashDriverDownload())
	{
		return FALSE;
	}

	*o_pbIsOperateFinsh = FALSE; 

	switch(Flash_GetCurEraseFlashStep())
	{
		case START_ERASE_FLASH:	
			/*get old app type*/
			s_appType = Flash_GetOldAPPType();

			s_pAppFlashMemoryInfo = NULL_PTR;
			s_appFlashItem = 0u;
			s_eraseSectorsCnt = 0u;

			s_result = TRUE;

			/*get old app type flash config*/
			if(TRUE == FLASH_HAL_GetFlashConfigInfo(s_appType, &s_pAppFlashMemoryInfo, &s_appFlashItem))
			{
				Flash_SetEraseFlashStep(DO_ERASING_FLASH);
			}
			
			break;


		case DO_ERASING_FLASH:
			/*get total sectors*/
//			totalSectors = FLASH_HAL_GetTotalSectors(s_appType);
//
//			/*one time erase all flash sectors*/
//			if(totalSectors <= maxEraseSectors)
//			{
//				while(s_appFlashItem)
//				{
//					eraseFlashLen = s_pAppFlashMemoryInfo->xBlockEndLogicalAddr -
//									s_pAppFlashMemoryInfo->xBlockStartLogicalAddr;
//
//					/*fed watchdog*/
//					WATCHDOG_HAL_Fed();
//
//					sectorNo = FLASH_HAL_GetFlashLengthToSectors(s_pAppFlashMemoryInfo->xBlockStartLogicalAddr, eraseFlashLen);
//
//					if(NULL_PTR != gs_stFlashDownloadInfo.stFlashOperateAPI.pfEraserSecotr)
//					{
//						/*disable all interrupts*/
//						DisableAllInterrupts();
//
//						eraseSectorNoTmp = sectorNo;
//
//						eraseFlashStartAddr = s_pAppFlashMemoryInfo->xBlockStartLogicalAddr;
//
//						/*erase a sector once because for watchdog*/
//						while(eraseSectorNoTmp)
//						{
//							/*fed watchdog*/
//							WATCHDOG_HAL_Fed();
//
//							/*do erase flash */
//							s_result = gs_stFlashDownloadInfo.stFlashOperateAPI.pfEraserSecotr(eraseFlashStartAddr, 1u);
//
//							eraseSectorNoTmp--;
//							if(TRUE != s_result)
//							{
//								break;
//							}
//
//							eraseFlashStartAddr += FLASH_HAL_Get1SectorBytes();
//						}
//
//						/*enable all all interrupts*/
//						EnableAllInterrupts();
//					}
//					else
//					{
//						s_result = FALSE;
//					}
//
//					if(TRUE != s_result)
//					{
//						break;
//					}
//
//					s_eraseSectorsCnt += sectorNo;
//
//					s_appFlashItem--;
//					s_pAppFlashMemoryInfo++;
//				}
//
//			}
//			else
//			{
//				while((s_eraseSectorsCnt < totalSectors) && (0u != s_appFlashItem))
//				{
//					/*fed watchdog*/
//					WATCHDOG_HAL_Fed();
//
//					/*get erase sector start address*/
//					if(TRUE != FLASH_HAL_SectorNumberToFlashAddress(s_appType, s_eraseSectorsCnt, &eraseFlashStartAddr))
//					{
//						s_result = FALSE;
//
//						break;
//					}
//
//                    /*check erase sector indicate flash address is valid or not?*/
//					if((eraseFlashStartAddr >= s_pAppFlashMemoryInfo->xBlockStartLogicalAddr) &&
//						(eraseFlashStartAddr < s_pAppFlashMemoryInfo->xBlockEndLogicalAddr))
//					{
//					    /*calculate length*/
//						eraseFlashLen = s_pAppFlashMemoryInfo->xBlockEndLogicalAddr -
//										eraseFlashStartAddr;
//					}
//					else
//					{
//						s_result = FALSE;
//
//						break;
//					}
//
//					/*save erase flash memeory address*/
//					//eraseFlashStartAddr = s_pAppFlashMemoryInfo->xBlockStartLogicalAddr;
//
//   					/*calculate can erase max sectors*/
//					canEraseMaxSectors = maxEraseSectors - (s_eraseSectorsCnt % maxEraseSectors);
//
//					/*calculate flash length to sectors*/
//					sectorNo = FLASH_HAL_GetFlashLengthToSectors(eraseFlashStartAddr, eraseFlashLen);
//					if(sectorNo > maxEraseSectors)
//					{
//						sectorNo = maxEraseSectors;
//						if(sectorNo > canEraseMaxSectors)
//						{
//							sectorNo = canEraseMaxSectors;
//						}
//					}
//					else
//					{
//						if(sectorNo <= canEraseMaxSectors)
//						{
//							s_appFlashItem--;
//							s_pAppFlashMemoryInfo++;
//						}
//						else
//						{
//							sectorNo = canEraseMaxSectors;
//						}
//					}
//
//				    /*erase flash memory*/
//					if(NULL_PTR != gs_stFlashDownloadInfo.stFlashOperateAPI.pfEraserSecotr)
//					{
//						/*disable all interrupts*/
//						DisableAllInterrupts();
//
//						eraseSectorNoTmp = sectorNo;
//
//						/*erase a sector once because for watchdog*/
//						while(eraseSectorNoTmp)
//						{
//							/*fed watchdog*/
//							WATCHDOG_HAL_Fed();
//
//							/*do erase flash */
//							s_result = gs_stFlashDownloadInfo.stFlashOperateAPI.pfEraserSecotr(eraseFlashStartAddr, 1u);
//
//							eraseSectorNoTmp--;
//							if(TRUE != s_result)
//							{
//								break;
//							}
//
//							eraseFlashStartAddr += FLASH_HAL_Get1SectorBytes();
//						}
//
//
//						/*enable all all interrupts*/
//						EnableAllInterrupts();
//					}
//					else
//					{
//						s_result = FALSE;
//					}
//
//					if(TRUE != s_result)
//					{
//						break;
//					}
//
//					/*add erased sectors count*/
//					s_eraseSectorsCnt += sectorNo;
//
//					/*if erase max Erase sectors and have some sectors wait to erase, then request time from host*/
//					if((0u == (s_eraseSectorsCnt % maxEraseSectors)) && (s_eraseSectorsCnt < totalSectors))
//					{
//						*o_pbIsOperateFinsh = FALSE;
//
//						break;
//					}
//
//				}
//
//			}
//
//			if((FALSE == *o_pbIsOperateFinsh) && (TRUE == s_result) && (s_eraseSectorsCnt < totalSectors))
//			{
//				Flash_RegInterruptedJob(FLASH_ERASING);
//				Flash_RestoreOperateFlashActiveJob(FLASH_WAITTING);
//
//				/*request more time from host*/
//				if(NULL_PTR != gs_stFlashDownloadInfo.pfRequestMoreTime)
//				{
//					gs_stFlashDownloadInfo.pfRequestMoreTime(gs_stFlashDownloadInfo.requestActiveJobUDSSerID, RequetMoreTimeSuccessfulFromHost);
//				}
//			}
//			else
//			{
//				s_result =gs_stFlashDownloadInfo.stFlashOperateAPI.pfEraserSecotr(eraseFlashStartAddr, 1u);

				ret = gs_stFlashDownloadInfo.stFlashOperateAPI.pfFlashUnlock(s_pAppFlashMemoryInfo->xBlockStartLogicalAddr, s_pAppFlashMemoryInfo->xBlockEndLogicalAddr);
				if(ret) {
					ret = gs_stFlashDownloadInfo.stFlashOperateAPI.pfFlashErase(s_pAppFlashMemoryInfo->xBlockStartLogicalAddr, s_pAppFlashMemoryInfo->xBlockEndLogicalAddr);
				}


//				ret = Flash_Unlock_g(0x00FF0000u, 0x01FFFFFFu);
//				ret = Flash_Erase_g(0x00FF0000u, 0x01FFFFFFu);

				//if((TRUE == s_result) && (s_eraseSectorsCnt == totalSectors))
				if(ret)
				{
					Flash_SetAPPStatus(TRUE, FALSE, TRUE);

					Flash_SetAPPTypeErased(s_appType);
				}
				else
				{
					Flash_SetAPPStatus(FALSE, FALSE, TRUE);

					Flash_ClearAPPTypeErased(s_appType);
				}

				Flash_CreateAndSaveAppStatusCrc(&xCountCrc);

				s_eraseSectorsCnt = 0u;
				
				Flash_SetEraseFlashStep(END_ERASE_FLASH);
			//}
			

			break;

		case END_ERASE_FLASH:
			
			WATCHDOG_HAL_Fed();
								
			s_pAppFlashMemoryInfo = NULL_PTR;
			s_appFlashItem = 0u;
			s_eraseSectorsCnt = 0u;

			*o_pbIsOperateFinsh = TRUE;

			Flash_SetEraseFlashStep(START_ERASE_FLASH);

			break;

		default:
			
			Flash_SetEraseFlashStep(START_ERASE_FLASH);
			
			break;
	}
	
	return s_result;
}


/*flash write */
static uint8 Flash_Write(boolean * o_pbIsOperateFinsh)
{
	uint8 result = FALSE;
	uint32 countCrc = 0u;
	uint8 flashDataIndex = 0u;
	uint8 fillCnt = 0u;

    /*check flash driver valid or not?*/
	if(TRUE != Flash_IsFlashDriverDownload())
	{
		return FALSE;
	}

	result = TRUE;
	while(gs_stFlashDownloadInfo.receiveProgramDataLength >= PROGRAM_SIZE)
	{
		/*count application flash crc*/
		Flash_CreateAppStatusCrc(&countCrc);
		if(TRUE != Flash_IsFlashAppCrcEqualStorage(countCrc))
		{			
			/*crc not right*/
			result = FALSE;
			
			break;
		}

		if((TRUE == Flash_IsFlashEraseSuccessful()) && 
		   (TRUE == Flash_IsFlashStructValid()))
		{
			WATCHDOG_HAL_Fed();
			
			/*write data in flash*/
			if(NULL_PTR != gs_stFlashDownloadInfo.stFlashOperateAPI.pfFlashProgram)
			{
				DisableAllInterrupts();
				result = gs_stFlashDownloadInfo.stFlashOperateAPI.pfFlashProgram(gs_stFlashDownloadInfo.startAddr,
													  (uint32*)&gs_stFlashDownloadInfo.aProgramDataBuff[flashDataIndex * PROGRAM_SIZE],
												      PROGRAM_SIZE);
				EnableAllInterrupts();
			}
			else
			{
				result = FALSE;
			}
			
			if(TRUE == result)
			{
				gs_stFlashDownloadInfo.length -= PROGRAM_SIZE;
				gs_stFlashDownloadInfo.receiveProgramDataLength -= PROGRAM_SIZE;
				gs_stFlashDownloadInfo.startAddr += PROGRAM_SIZE;

				flashDataIndex++;
			}
			else
			{
				result = FALSE;
				
				break;
			}
		}
		else
		{
			result = FALSE;
			
			break;
		}
	}

	/*calculate if program data is align < 8 bytes, need to fill 0xFF to align 8 bytes.*/
	if((0u != gs_stFlashDownloadInfo.receiveProgramDataLength) && (TRUE == result))
	{
		fillCnt = (uint8)(gs_stFlashDownloadInfo.receiveProgramDataLength & 0x07u);
		fillCnt = (~fillCnt + 1u) & 0x07u;

		fsl_memset((void *)&gs_stFlashDownloadInfo.aProgramDataBuff[flashDataIndex * PROGRAM_SIZE + gs_stFlashDownloadInfo.receiveProgramDataLength], 
					0xFFu, 
					fillCnt);

		gs_stFlashDownloadInfo.receiveProgramDataLength += fillCnt;
	
		/*write data in flash*/
		if(NULL_PTR != gs_stFlashDownloadInfo.stFlashOperateAPI.pfFlashProgram)
		{
		
			//DisableAllInterrupts();
			result = gs_stFlashDownloadInfo.stFlashOperateAPI.pfFlashProgram(gs_stFlashDownloadInfo.startAddr,
												  (uint32 *)&gs_stFlashDownloadInfo.aProgramDataBuff[flashDataIndex * PROGRAM_SIZE],
												  gs_stFlashDownloadInfo.receiveProgramDataLength / 4);
			//EnableAllInterrupts();
		}
		else
		{
			result = FALSE;
		}
		
		if(TRUE == result)
		{
			gs_stFlashDownloadInfo.length -= (gs_stFlashDownloadInfo.receiveProgramDataLength - fillCnt);
			gs_stFlashDownloadInfo.startAddr += gs_stFlashDownloadInfo.receiveProgramDataLength;
			gs_stFlashDownloadInfo.receiveProgramDataLength = 0;
		
			flashDataIndex++;
		}
		
	}

	if(TRUE == result)
	{
		Flash_SetFlashProgramStatus(TRUE);

		Flash_CreateAndSaveAppStatusCrc(&countCrc);
		
		return TRUE;
	}

	Flash_InitDowloadInfo();

	return FALSE;
}

/*get checksum step*/
static tChecksumStep Flash_GetChecksumStep(void)
{
	return gs_eChecksumStep;
}

/*set checksum step*/
static void Flash_SetChecksumStep(const tChecksumStep i_checksumStep)
{
	gs_eChecksumStep = (tChecksumStep)i_checksumStep;
}

/*flash check sum */
static uint8 Flash_Checksum(boolean * o_pbIsOperateFinsh)
{
#if 0
	tCrc xCountCrc = 0u;

	WATCHDOG_HAL_Fed();

    /*reserved the if and else for external flash memory, like external flash need to flash dirver read or write.*/ 
	if((TRUE == Flash_IsFlashDriverSoftwareData()))
	{
		CRC_HAL_CalculateCRCOnce((const uint8 *)gs_stFlashDownloadInfo.receivedDataStartAddr, gs_stFlashDownloadInfo.receivedDataLength, &xCountCrc);
    }
	/*Only flash driver is download, can do erase/write flash */
	else if(TRUE == Flash_IsFlashDriverDownload())	
	{
		CRC_HAL_StartSoftwareCrc(&xCountCrc);
		CRC_HAL_CreatHardwareCrc((const uint8 *)gs_stFlashDownloadInfo.receivedDataStartAddr, gs_stFlashDownloadInfo.receivedDataLength, &xCountCrc);
		CRC_HAL_EndSoftwareCrc(&xCountCrc);
		
		gs_stFlashDownloadInfo.receivedDataStartAddr += gs_stFlashDownloadInfo.receivedDataLength;
		gs_stFlashDownloadInfo.receivedDataLength = 0u;
	}
	else
	{
		/*do nothing*/
	}

	/*fed watchdog*/
	WATCHDOG_HAL_Fed();
#endif

	boolean isCheckSuccessful = FALSE;

	/*
	* flash driver data or APP data (flash driver is downloaded) can be calculate CRC
	*/
	if((TRUE == Flash_IsFlashDriverSoftwareData()) ||
	   ((TRUE != Flash_IsFlashDriverSoftwareData()) && (TRUE == Flash_IsFlashDriverDownload())))
	{
		/*check crc value*/
		isCheckSuccessful = Flash_DoChecksum(o_pbIsOperateFinsh);
	}
	else
	{
		*o_pbIsOperateFinsh = TRUE;
		isCheckSuccessful = FALSE;
	}
	
	/*check operate finsh or not*/
	if(TRUE == *o_pbIsOperateFinsh)
	{
		/*check CRC valid or not*/
		if(TRUE == Flash_IsReceivedCRCValid())	
		{
			if((TRUE == Flash_IsFlashDriverSoftwareData()))
			{
				Flash_SetFlashDriverDowload();

				if(TRUE != FLASH_HAL_RegisterFlashAPI(&gs_stFlashDownloadInfo.stFlashOperateAPI))
				{
					Flash_SetFlashDriverNotDonwload();
				}
				else
				{
					if(NULL_PTR != gs_stFlashDownloadInfo.stFlashOperateAPI.pfFlashInit)
					{
						gs_stFlashDownloadInfo.stFlashOperateAPI.pfFlashInit(); 
					}
				}
			}
			
			isCheckSuccessful = TRUE;
		}
		else
		{	
			FLS_DebugPrintf("%s: CRC failed!\n", __func__);
			
			isCheckSuccessful = FALSE;
		}
	}
	else
	{
		isCheckSuccessful = TRUE;
	}

	return isCheckSuccessful;
}

/*do checksum*/

static boolean Flash_DoChecksum(boolean *o_pbIsOperateFinsh)
{
	tChecksumStep checksumStep = START_CHECKSUM;
	uint8 isSuccessful = FALSE;
	static tCrc s_xCountCrc = 0u;
	boolean isOperateFinsh = FALSE;
	static uint32 s_calCRCDataLen = 0u;
	static uint32 startCalDataAddr = 0u;
	uint32 calCRCDataLen = 0u;
	
	WATCHDOG_HAL_Fed();

	*o_pbIsOperateFinsh = FALSE;

	isSuccessful = TRUE;
	checksumStep = Flash_GetChecksumStep();
	switch(checksumStep)
	{
		case START_CHECKSUM:
			
			CRC_HAL_StartSoftwareCrc(&s_xCountCrc);
			s_calCRCDataLen = 0u;
			Flash_SetChecksumStep(DO_CHECKING_CHECKSUM);

			break;

		case DO_CHECKING_CHECKSUM:

			if((gs_stFlashDownloadInfo.receivedDataLength - s_calCRCDataLen) >= MAX_ONCE_CHECKSUM_SIZE)
			{
				calCRCDataLen = MAX_ONCE_CHECKSUM_SIZE;
			}
			else
			{
				calCRCDataLen = gs_stFlashDownloadInfo.receivedDataLength - s_calCRCDataLen;
			}

			startCalDataAddr = gs_stFlashDownloadInfo.receivedDataStartAddr + s_calCRCDataLen;
			
			CRC_HAL_CreatSoftwareCrc((const uint8 *)startCalDataAddr, calCRCDataLen, &s_xCountCrc);

			s_calCRCDataLen += calCRCDataLen;
			if(s_calCRCDataLen == gs_stFlashDownloadInfo.receivedDataLength)
			{
				Flash_SetChecksumStep(END_CHECKSUM);
			}
			else
			{
				/*request more time*/
				Flash_RegInterruptedJob(FLASH_CHECKING);
				Flash_RestoreOperateFlashActiveJob(FLASH_WAITTING);
				
				/*request more time from host*/
				if(NULL_PTR != gs_stFlashDownloadInfo.pfRequestMoreTime)
				{
					gs_stFlashDownloadInfo.pfRequestMoreTime(gs_stFlashDownloadInfo.requestActiveJobUDSSerID, RequetMoreTimeSuccessfulFromHost);
				}				
			}

			break;

		case END_CHECKSUM:
			
			CRC_HAL_EndSoftwareCrc(&s_xCountCrc);
			Flash_SaveCalculateCRCValue(s_xCountCrc);
			isOperateFinsh = TRUE;

			Flash_SetChecksumStep(START_CHECKSUM);
			break;

		default:

			Flash_SetChecksumStep(START_CHECKSUM);
			isSuccessful = FALSE;
			isOperateFinsh = TRUE;
			
			break;
	}

	WATCHDOG_HAL_Fed();

	*o_pbIsOperateFinsh = isOperateFinsh;

	if(TRUE != isSuccessful)
	{
		FLS_DebugPrintf("%s: checksum job failed!\n", __func__);
		
		return isSuccessful;
	}

	return isSuccessful;	
}

/*save received check sum crc*/
void Flash_SavedReceivedCheckSumCrc(uint32 i_receivedCrc)
{
	gs_stFlashDownloadInfo.receivedCRC = (tCrc)i_receivedCrc;
}

/*Is received CRC valid?*/
static boolean Flash_IsReceivedCRCValid(void)
{
	boolean res = FALSE;

#ifdef DebugBootloader_NOTCRC
	res = TRUE;
#else
	if(gs_stFlashDownloadInfo.receivedCRC == gs_stFlashDownloadInfo.calculateCRCValue)
	{
		res = TRUE;
	}
#endif

	return res;
}

/*save calcualte CRC value*/
static void Flash_SaveCalculateCRCValue(const uint32 i_CRCValue)
{
	gs_stFlashDownloadInfo.calculateCRCValue = (uint32)i_CRCValue;
}

/*Flash program region. Called by uds servive 0x36u*/
uint8 Flash_ProgramRegion(const uint32 i_addr,
									  const uint8 *i_pDataBuf, 
							          const uint32 i_dataLen)
{
	uint8 dataLen = (uint8)i_dataLen;
	uint8 result = TRUE;
	
	ASSERT(NULL_PTR == i_pDataBuf);

	result = TRUE;
	if(FL_TRANSFER_STEP != Flash_GetCurDownloadStep())
	{
		result = FALSE;
	}

	/*saved flash data*/
	if(TRUE != Flash_SavedFlashData(i_pDataBuf, dataLen))
	{
		result = FALSE;
	}

	if(TRUE == result)
	{
		if((FALSE == Flash_IsFlashDriverDownload()) || (TRUE == Flash_IsFlashDriverSoftwareData()))
		{
			/*if flash driver, copy the data to RAM*/
			if(TRUE == Flash_IsFlashDriverSoftwareData())
			{
				fsl_memcpy((void *)i_addr,(void *)i_pDataBuf,dataLen);
			}

			Flash_SetOperateFlashActiveJob(FLASH_IDLE, NULL_PTR, INVALID_UDS_SERVICES_ID, NULL_PTR);
		}
		else
		{
			Flash_SetOperateFlashActiveJob(FLASH_PROGRAMMING, NULL_PTR, INVALID_UDS_SERVICES_ID, NULL_PTR);
			
			gs_stFlashDownloadInfo.errorCode = TRUE;
		}	
	}
	
	/*received erro data.*/
	if(TRUE != result)
	{
		Flash_InitDowloadInfo();
	}
	
	return result;
}

/*get rest hander address*/
uint32 Flash_GetResetHandlerAddr(void)
{
	return gs_stAppFlashStatus.appStartAddr;
}

/*Is flash driver software data?*/
static boolean Flash_IsFlashDriverSoftwareData(void)
{
	uint32 flashDriverStartAddr = 0u;
	uint32 flashDriverEndAddr = 0u;
	boolean result = FALSE;


	result = FLASH_HAL_GetFlashDriverInfo(&flashDriverStartAddr, &flashDriverEndAddr);

	if((gs_stFlashDownloadInfo.startAddr >= flashDriverStartAddr) && 
            ((gs_stFlashDownloadInfo.startAddr + gs_stFlashDownloadInfo.length) < flashDriverEndAddr) &&
            (TRUE == result))
	{
		result =  TRUE;
    }
    else
    {
        result = FALSE;
    }

	return result;
}

/*read application informaiton from flash*/
static void Flash_ReadNewestAppInfoFromFlash(void)
{
	tAPPType newestAppType = APP_A_TYPE;
	uint32 appInfoStart = 0u;
	uint32 appInfoBlocksize = 0u;
	boolean result = FALSE;
	
	newestAppType = Flash_GetNewestAPPType();

	result = FLASH_HAL_GetAPPInfo(newestAppType, &appInfoStart, &appInfoBlocksize);

	if((sizeof(tAppFlashStatus) <= appInfoBlocksize) && (TRUE == result))
	{
		result = Flash_SaveAppInfoData(appInfoStart);
	}
}

/*save app info data*/
static boolean Flash_SaveAppInfoData(const uint32 i_appInfoDataStartAddr)
{
	boolean res = FALSE;
#ifdef EN_APP_INFO_DATA_IN_NONE_FLASH
	res = FLASH_HAL_ReadAPPInfoData(i_appInfoDataStartAddr, (sizeof(tAppFlashStatus)), &gs_stAppFlashStatus);
#else
	gs_stAppFlashStatus = *(tAppFlashStatus*)i_appInfoDataStartAddr;
	res = TRUE;
#endif

	return res;
}

/*Is read application inforemation from flash valid?*/
uint8 Flash_IsReadAppInfoFromFlashValid(void)
{
	tCrc xCrc = 0u;

	/*read application information from flash*/
	Flash_ReadNewestAppInfoFromFlash();
	
	Flash_CreateAppStatusCrc(&xCrc);

	return Flash_IsFlashAppCrcEqualStorage(xCrc);

}

/*Is application in flash valid? If valid return TRUE, else return FALSE.*/
uint8 Flash_IsAppInFlashValid(void)
{
	if(((TRUE == Flash_IsFlashProgramSuccessful()) &&
		(TRUE == Flash_IsFlashEraseSuccessful())) &&
	   (TRUE == Flash_IsFlashStructValid()))
	{
		return TRUE;
	}

	return FALSE;
}

/*erase flash driver in RAM*/
void Flash_EraseFlashDriverInRAM(void)
{
	uint32 flashDriverStartAddr = 0u;
	uint32 flashDriverEndAddr = 0u;
	boolean result = FALSE;

	result = FLASH_HAL_GetFlashDriverInfo(&flashDriverStartAddr, &flashDriverEndAddr);

	if(TRUE == result)
	{
		fsl_memset((void *)flashDriverStartAddr, 0x0u, flashDriverEndAddr - flashDriverStartAddr);
	}
}

/*save printfigner*/
void Flash_SavePrintfigner(const uint8 *i_pPrintfigner, const uint8 i_printfinerLen)
{

	uint8 printfignerLen = 0u;
	tCrc crc = 0u;
	
	ASSERT(NULL_PTR == i_pPrintfigner);

	if(i_printfinerLen > FL_FINGER_PRINT_LENGTH)
	{
		printfignerLen = FL_FINGER_PRINT_LENGTH;
	}
	else
	{
		printfignerLen =  (uint8)i_printfinerLen;
	}

	fsl_memcpy((void *)gs_stFlashDownloadInfo.pstAppFlashStatus->aFingerPrint,(const void *)i_pPrintfigner,printfignerLen);

	Flash_CreateAndSaveAppStatusCrc(&crc);
}

/*write flash application information called by bootloader last step*/
uint8 Flash_WriteFlashAppInfo(void)
{
	uint8 result = FALSE;
	tAPPType oldAppType = APP_A_TYPE;
	uint32 appInfoStartAddr = 0u;
	uint32 appInfoLen = 0u;
	uint32 crc = 0u;
	tAppFlashStatus *pAppStatusPtr = NULL_PTR;

	tAPPType newestAPPType = APP_A_TYPE;
	uint32 newestAPPInfoStartAddr = 0u;
	uint32 newestAPPInfoLen = 0u;
	tAppFlashStatus *pstNewestAPPFlashStatus = NULL_PTR;
	volatile uint32 resetHandleAddr = 0u;

	boolean bIsEnableWriteResetHandle = FALSE;
	uint32 resetHandleOffset = 0u;
	uint32 resetHandleLength = 0u;

	Flash_CreateAndSaveAppStatusCrc(&crc);
	
	oldAppType = Flash_GetOldAPPType();
	newestAPPType = Flash_GetNewestAPPType();
	
	result = FLASH_HAL_GetAPPInfo(oldAppType, &appInfoStartAddr, &appInfoLen);
	if(TRUE == result)
	{
		/*write data information in flash*/
		pAppStatusPtr = Flash_GetAppStatusPtr();

		FLASH_HAL_GetRestHanlderInfo(&bIsEnableWriteResetHandle, &resetHandleOffset, &resetHandleLength);

		/*update APP cnt*/
		if(TRUE == FLASH_HAL_GetAPPInfo(newestAPPType, &newestAPPInfoStartAddr, &newestAPPInfoLen))
		{
			pstNewestAPPFlashStatus = (tAppFlashStatus *)newestAPPInfoStartAddr;

			pAppStatusPtr->appCnt = pstNewestAPPFlashStatus->appCnt + 1u;
			if(0xFFu == pAppStatusPtr->appCnt)
			{
				pAppStatusPtr->appCnt = 0u;
			}

#ifdef EN_APP_INFO_DATA_IN_NONE_FLASH
			resetHandleAddr = resetHandleOffset;
#else
			/*get app start address from flash. The address is the newest APP, because the APP info not write in flash, so the APP is old*/
			resetHandleAddr = appInfoStartAddr + resetHandleOffset;
#endif
			Flash_SaveAppResetHandlerAddr(resetHandleAddr, resetHandleLength);

			FLS_DebugPrintf("APP type =%X, APP address=0x%X\n", oldAppType, *((uint32*)resetHandleAddr));

			crc = 0u;
			Flash_CreateAndSaveAppStatusCrc(&crc);
		}	

		if(NULL_PTR != pAppStatusPtr)
		{
			result = FALSE;
			
#ifdef EN_APP_INFO_DATA_IN_NONE_FLASH
			/*APP information storage in EEPROM or DFLASH*/
			result = FLASH_HAL_WriteAPPInfoData(appInfoStartAddr,
											   (uint8 *)pAppStatusPtr,
											   sizeof(tAppFlashStatus));
#else
			if(NULL_PTR != gs_stFlashDownloadInfo.stFlashOperateAPI.pfFlashProgram)
			{
				result = gs_stFlashDownloadInfo.stFlashOperateAPI.pfFlashProgram(appInfoStartAddr,
				                                    (uint32 *)pAppStatusPtr,
				                                    sizeof(tAppFlashStatus));
			}
#endif			
		}
		else
		{
			result = FALSE;
		}
	}

	return result;
}

#ifdef EN_SUPPORT_APP_B
static tAPPType Flash_DoCheckNewestAPPCnt(const tAppFlashStatus * i_pAppAInfo, const tAppFlashStatus *i_pAppBInfo)
{
	uint8 appACnt = 0u;
	uint8 appBCnt = 0u;
	uint8 deltaCnt = 0u;
	tAPPType newestAPP = APP_A_TYPE;

	ASSERT(NULL_PTR == i_pAppAInfo);
	ASSERT(NULL_PTR == i_pAppBInfo);

	appACnt = i_pAppAInfo->appCnt;
	appBCnt = i_pAppBInfo->appCnt;

	deltaCnt = (appACnt > appBCnt) ? (appACnt - appBCnt) : (appBCnt - appACnt);

	if(1u == deltaCnt)
	{
		if(appACnt > appBCnt)
		{
			newestAPP = APP_A_TYPE;
		}
		else
		{
			newestAPP = APP_B_TYPE;
		}
	}
	else if(0xFEu == deltaCnt)
	{
		if(appACnt < appBCnt)
		{
			newestAPP = APP_A_TYPE;
		}
		else
		{
			newestAPP = APP_B_TYPE;
		}		
	}
	else
	{
		/*when cnt = 0xFF, then current cnt is invalid*/
		if((0xFFu == appACnt) && (0xFFu != appBCnt))
		{
			newestAPP = APP_B_TYPE;
		}
		else if((0xFFu != appACnt) && (0xFFu == appBCnt))
		{
			newestAPP = APP_A_TYPE;
		}
		else if((0xFFu == appACnt) && (0xFFu == appBCnt))
		{
			/*invalid conter*/
			newestAPP = APP_INVLID_TYPE;
		}
		else
		{
			newestAPP = APP_A_TYPE;
		}
	}

	return newestAPP;
}

/*Do check newest app information*/
static tAPPType Flash_DoCheckNewestAPPInfo(const tAppFlashStatus * i_pAppAInfo, const tAppFlashStatus *i_pAppBInfo)
{
#ifdef EN_SUPPORT_APP_B
	uint32 crc = 0u;
	boolean bIsAppAValid = FALSE;
	boolean bIsAppBValid = FALSE;
	tAPPType newestAPP = APP_A_TYPE;
#endif /*#ifndef EN_SUPPORT_APP_B*/

	ASSERT(NULL_PTR == i_pAppAInfo);
	ASSERT(NULL_PTR == i_pAppBInfo);

#ifndef EN_SUPPORT_APP_B
	return APP_A_TYPE;
#else /*#ifndef EN_SUPPORT_APP_B*/

	CRC_HAL_CalculateCRCOnce((const uint8 *)i_pAppAInfo, sizeof(tAppFlashStatus) - 4u, &crc);
	if(crc == i_pAppAInfo->crc)
	{
		bIsAppAValid = TRUE;
	}

	CRC_HAL_CalculateCRCOnce((const uint8 *)i_pAppBInfo, sizeof(tAppFlashStatus) - 4u, &crc);
	if(crc == i_pAppBInfo->crc)	
	{
		bIsAppBValid = TRUE;
	}

	if((TRUE == bIsAppAValid) && (TRUE != bIsAppBValid))
	{
		newestAPP = APP_A_TYPE;
	}
	else if((TRUE != bIsAppAValid) && (TRUE == bIsAppBValid))
	{
		newestAPP = APP_B_TYPE;
	}
	else if((TRUE != bIsAppAValid) && (TRUE != bIsAppBValid))
	{
		newestAPP = APP_A_TYPE;
	}
	else
	{
		/*check APP A and B who is newest, both APP A & B is valid*/
		newestAPP = Flash_DoCheckNewestAPPCnt(i_pAppAInfo, i_pAppBInfo);
	}

	return newestAPP;
#endif	/*#ifndef EN_SUPPORT_APP_B*/
	
}

#endif /*#ifdef EN_SUPPORT_APP_B*/
/*Get newest app info*/
tAPPType Flash_GetNewestAPPType(void)
{
#ifdef EN_SUPPORT_APP_B
	uint32 appInfoStartAddr_A = 0u;
	uint32 appInfoBlockSize_A = 0u;
	tAPPType newestAPP = APP_A_TYPE;

	tAppFlashStatus appAInfo;
	tAppFlashStatus appBInfo;

	uint32 appInfoStartAddr_B = 0u;
	uint32 appInfoBlockSize_B = 0u;
#endif /*#ifndef EN_SUPPORT_APP_B*/

/*don't support APP B, so APP A is always newest.*/
#ifndef EN_SUPPORT_APP_B
	return APP_A_TYPE;
#else

	FLASH_HAL_GetAPPInfo(APP_A_TYPE, &appInfoStartAddr_A, &appInfoBlockSize_A);

	FLASH_HAL_GetAPPInfo(APP_B_TYPE, &appInfoStartAddr_B, &appInfoBlockSize_B);

#ifdef EN_APP_INFO_DATA_IN_NONE_FLASH
	FLASH_HAL_ReadAPPInfoData(appInfoStartAddr_A, appInfoBlockSize_A, (uint8 *)&appAInfo);

	FLASH_HAL_ReadAPPInfoData(appInfoStartAddr_B, appInfoBlockSize_B, (uint8 *)&appBInfo);
#else
	/*read APP A info*/
	appAInfo = *(tAppFlashStatus *)appInfoStartAddr_A;

    /*read APP B info*/
	appBInfo = *(tAppFlashStatus *)appInfoStartAddr_B;
#endif

	newestAPP = Flash_DoCheckNewestAPPInfo(&appAInfo, &appBInfo);

	return newestAPP;
#endif /*#ifndef EN_SUPPORT_APP_B*/
}

/*Get old app info*/
tAPPType Flash_GetOldAPPType(void)
{
#ifdef EN_SUPPORT_APP_B
	uint32 appInfoStartAddr_A = 0u;
	uint32 appInfoBlockSize_A = 0u;
	tAPPType oldAPP = APP_A_TYPE;

	uint32 appInfoStartAddr_B = 0u;
	uint32 appInfoBlockSize_B = 0u;
	
	tAPPType newestAPP = APP_A_TYPE;

	tAppFlashStatus appAInfo;
	tAppFlashStatus appBInfo;	
#endif /*#ifndef EN_SUPPORT_APP_B*/

/*don't support APP B, so APP A is always old.*/
#ifndef EN_SUPPORT_APP_B
	return APP_A_TYPE;
#else

	FLASH_HAL_GetAPPInfo(APP_A_TYPE, &appInfoStartAddr_A, &appInfoBlockSize_A);

	FLASH_HAL_GetAPPInfo(APP_B_TYPE, &appInfoStartAddr_B, &appInfoBlockSize_B);

#ifdef EN_APP_INFO_DATA_IN_NONE_FLASH
	FLASH_HAL_ReadAPPInfoData(appInfoStartAddr_A, appInfoBlockSize_A, (uint8 *)&appAInfo);

	FLASH_HAL_ReadAPPInfoData(appInfoStartAddr_B, appInfoBlockSize_B, (uint8 *)&appBInfo);
#else
	/*read APP A info*/
	appAInfo = *(tAppFlashStatus *)appInfoStartAddr_A;

    /*read APP B info*/
	appBInfo = *(tAppFlashStatus *)appInfoStartAddr_B;
#endif

	newestAPP = Flash_DoCheckNewestAPPInfo(&appAInfo, &appBInfo);
	if(APP_A_TYPE == newestAPP)
	{
		oldAPP = APP_B_TYPE;
	}
	else if(APP_B_TYPE == newestAPP)
	{
		oldAPP = APP_A_TYPE;
	}
	else
	{
		/*here is set old APP type is default, just for error*/
		oldAPP = APP_A_TYPE; 
	}

	return oldAPP;
#endif	/*#ifndef EN_SUPPORT_APP_B*/
}

/*Is valid address?*/
boolean Flash_IsValidAddr(const tAPPType i_appType, const uint32 i_dataAddr, const uint32 i_dataLen)
{
	boolean res = FALSE;
	BlockInfo_t* pBlockInfo = NULL_PTR;
	uint32 item = 0u;
	uint32 index = 0u;

	res = FLASH_HAL_GetFlashConfigInfo(i_appType, &pBlockInfo, &item);
	if(TRUE == res)
	{
		while(index < item)
		{
			if((i_dataAddr >= pBlockInfo->xBlockStartLogicalAddr) && 
				(i_dataAddr <= pBlockInfo->xBlockEndLogicalAddr))
			{
				if(((i_dataAddr + i_dataLen) >= pBlockInfo->xBlockStartLogicalAddr) &&
					((i_dataAddr + i_dataLen) <= pBlockInfo->xBlockEndLogicalAddr))
				{
					res = TRUE;					
				}
				else
				{
					res = FALSE;
				}

				break;
			}
			else
			{
				res = FALSE;
			}

			index++;
		}
	}

	return res;
}

/*Is flash driver data?*/
boolean Flash_IsFlashDriverData(const uint32 i_dataAddr, const uint32 i_dataLen)
{
	uint32 flashDrvStartAddr = 0u;
	uint32 flashDrvEndAddr = 0u;
	boolean res = FALSE;
	
	res = FLASH_HAL_GetFlashDriverInfo(&flashDrvStartAddr, &flashDrvEndAddr);
	if(TRUE == res)
	{
		if((i_dataAddr >= flashDrvStartAddr) && ((i_dataAddr + i_dataLen) <= flashDrvEndAddr))
		{
			res = TRUE;
		}
		else
		{
			res = FALSE;
		}
	}
	
	return res;
}

/*Set app type ersed*/
static void Flash_SetAPPTypeErased(const tAPPType i_erasedAPPType)
{
	if((APP_A_TYPE == i_erasedAPPType)
#ifdef EN_SUPPORT_APP_B
	    || (APP_B_TYPE == i_erasedAPPType)
#endif
	)
	{
		gs_stFlashDownloadInfo.erasedAPPFlag = 1u << i_erasedAPPType;
	}
}

/*clear app type erased status*/
static void Flash_ClearAPPTypeErased(const tAPPType i_erasedAPPType)
{
	if((APP_A_TYPE == i_erasedAPPType)
#ifdef EN_SUPPORT_APP_B
		|| (APP_B_TYPE == i_erasedAPPType)
#endif
	)
	{
		gs_stFlashDownloadInfo.erasedAPPFlag &= ~(1u << i_erasedAPPType);
	}
	else
	{
		/*set all app type was non erased.*/
		gs_stFlashDownloadInfo.erasedAPPFlag = 0u;
	}
}

/*Is APP type erased?*/
boolean Flash_IsAPPTypeErased(const tAPPType i_erasedAPPType)
{
	boolean res = FALSE;

	if((1u << i_erasedAPPType) & gs_stFlashDownloadInfo.erasedAPPFlag)
	{
		res = TRUE;
	}

	return res;
}

/**************************End file********************************/


