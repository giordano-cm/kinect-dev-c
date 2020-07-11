/**
 * @author  : Giordano Cechet Moro
 * @date    : 13 / 06 / 2020
 * @brief   : Código em C para trabalhar com Kinect 360 V1
 *
 * Para funcionar: lembre-se de inserir "-lfreenect_sync" como argumento do compilador
**/

//========================================================================
//  Cross-plataform Includes
//========================================================================
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

//========================================================================
//  Includes
//========================================================================
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <libfreenect.h>
#include <libfreenect_sync.h>

//========================================================================
//  Defines
//========================================================================
#define MY_KINECT_INDEX             0   /* Apenas 1 kinkect sendo utilizado */
#define TIME_TO_WAIT_US             10000
#define INITIAL_KINECT_MOTOR_ANGLE  (-30)
#define KINECT_ANGLE_DIR_UP         0
#define KINECT_ANGLE_DIR_DOWN       1
#define INITIAL_KINECT_ANGLE_DIR    KINECT_ANGLE_DIR_UP
#define NUM_OF_SEMI_CYCLES          1
#define RES_NUM_OF_LINES_PX         480
#define RES_NUM_OF_COLUMNS_PX       640
#define MIDDLE_POINT_OF_CAMERA      (((RES_NUM_OF_COLUMNS_PX/2)-1)+(((RES_NUM_OF_LINES_PX/2)-1)*RES_NUM_OF_COLUMNS_PX))

#define FILE_NAME           "linear_scanner.txt"
#define FILE_HEADER_STR     "index,depth_value,angle,direction"

//========================================================================
//  Global Variables
//========================================================================
uint8_t Semi_Cycles = 0;
freenect_raw_tilt_state * Device_State = NULL;
int8_t Device_Angle = INITIAL_KINECT_MOTOR_ANGLE;           /* de {-30,-29,-28,...,+28,+29,+30}  */
uint8_t Device_Angle_Direction = INITIAL_KINECT_ANGLE_DIR;  /* 0 - sobe(aumenta); 1 - desce(diminui) */
uint8_t Flag__Release = 0;
short * Depth = NULL;
uint32_t Timestamp = 0;

//========================================================================
//  Functions Prototypes
//========================================================================
void KINECT__Init();
void FILE__Write( void * data );
void APP__Init();
void APP__Thread( void * arg );
void APP__Deinit();

/*
    main
    Giordano C Moro
    xx/xx/2020
*/
int main()
{
    /* Inicializa a aplicação */
    APP__Init();

    /* Criação de threads */
    APP__Thread(NULL);

    /* Encerra o Kinect */
    APP__Deinit();

    printf("\n");
    return EXIT_SUCCESS;
}

/*
    KINECT__Init
    Giordano C Moro
    28/06/2020
*/
void KINECT__Init()
{
    /* Sets iniciais */
    freenect_sync_set_tilt_degs(INITIAL_KINECT_MOTOR_ANGLE, MY_KINECT_INDEX);
	freenect_sync_set_led(LED_GREEN, MY_KINECT_INDEX);

    /* Espera o KINECT terminar o movimento */
    do
    {
        freenect_sync_get_tilt_state(&Device_State, MY_KINECT_INDEX);
    } while ( Device_State->tilt_status == TILT_STATUS_MOVING );
}

/*
    FILE__Write
    Giordano C Moro
    28/06/2020
*/
void FILE__Write( void * data )
{
	FILE * fp = NULL;           /* Criação e escrita em arquivo */
	static uint32_t index = 0;
	short * data_ = (short *)data;

    /* Abre o arquivo para ler */
	fp = fopen(FILE_NAME, "r");

	/* Caso o arquivo não existe, é criado um com header */
	if ( fp == NULL )
	{
        fp = fopen(FILE_NAME, "w");
        fprintf(fp, "%s", FILE_HEADER_STR);
	}
	else
	{
        fclose(fp);
        fp = fopen(FILE_NAME, "a");
	}

    fprintf(fp, "\n%u,%d,%i,%d", index++, *data_, Device_Angle, Device_Angle_Direction);

    /* Fecha o arquivo */
	fclose(fp);
}

/*
    APP__Init
    Giordano C Moro
    30/06/2020
*/
void APP__Init()
{
	/* Inicializa o KINECT */
    KINECT__Init();
}

/*
    APP__Thread
    Giordano C Moro
    08/07/2020
*/
void APP__Thread( void * arg )
{
    /* Loop infinito */
    while ( Semi_Cycles < NUM_OF_SEMI_CYCLES )
    {
        /* Pega o valor da profundidade */
        freenect_sync_get_depth((void **)&Depth, &Timestamp, MY_KINECT_INDEX, FREENECT_DEPTH_11BIT);

        /* Muda o ângulo de inclinação */
        if ( Device_Angle_Direction == KINECT_ANGLE_DIR_UP )
        {
            if ( Device_Angle >= 30 )
            {
                Device_Angle_Direction = KINECT_ANGLE_DIR_DOWN;
                Device_Angle--;
                Semi_Cycles++;
            }
            else
            {
                Device_Angle++;
            }
        }
        else    /* KINECT_ANGLE_DIR_DOWN */
        {
            if ( Device_Angle <= -30 )
            {
                Device_Angle_Direction = KINECT_ANGLE_DIR_UP;
                Device_Angle++;
                Semi_Cycles++;
            }
            else
            {
                Device_Angle--;
            }
        }

        printf("\nÂngulo: (%d°) | Direção: (%s) | Depth: (%d)", Device_Angle, (Device_Angle_Direction == KINECT_ANGLE_DIR_UP ? "subindo" : "descendo"), Depth[MIDDLE_POINT_OF_CAMERA]);
        FILE__Write( &Depth[MIDDLE_POINT_OF_CAMERA] );
        fflush(stdout);
        freenect_sync_set_tilt_degs(Device_Angle, MY_KINECT_INDEX);

        /* Espera o KINECT terminar o movimento */
        do
        {
            freenect_sync_get_tilt_state(&Device_State, MY_KINECT_INDEX);
        } while ( Device_State->tilt_status == TILT_STATUS_MOVING );
    }
}

/*
    APP__Init
    Giordano C Moro
    30/06/2020
*/
void APP__Deinit()
{
    /* Desinicializa o KINECT */
    //KINECT__Deinit();
}
