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
#include <signal.h>
#include <libfreenect.h>
#include <libfreenect_sync.h>

//========================================================================
//  Defines
//========================================================================
#define MY_KINECT_INDEX             0   /* Apenas 1 kinkect sendo utilizado */
#define TIME_TO_WAIT_US             1000000
#define INITIAL_KINECT_MOTOR_ANGLE  (-30)
#define KINECT_ANGLE_DIR_UP         0
#define KINECT_ANGLE_DIR_DOWN       1
#define INITIAL_KINECT_ANGLE_DIR    KINECT_ANGLE_DIR_UP
#define NUM_OF_SEMI_CYCLES          3

#define FILE_NAME           "linear_scanner.txt"
#define FILE_HEADER_STR     "index,depth_value,angle,direction"

//========================================================================
//  Global Variables
//========================================================================
freenect_context * fn_ctx = NULL;
freenect_device * fn_dvc = NULL;
uint8_t Semi_Cycles = 0;
freenect_raw_tilt_state * Device_State = NULL;
int8_t Device_Angle = INITIAL_KINECT_MOTOR_ANGLE;           /* de {-30,-29,-28,...,+28,+29,+30}  */
uint8_t Device_Angle_Direction = INITIAL_KINECT_ANGLE_DIR;  /* 0 - sobe(aumenta); 1 - desce(diminui) */
//uint8_t * depth = NULL;                                       /* Profundidade */

//========================================================================
//  Functions Prototypes
//========================================================================
void KINECT__Init();
void KINECT__Depth_Callback(freenect_device * dvc, void * v_depth, uint32_t timestamp);
void KINECT__Deinit();
void FILE__Write( void * data );
//void INThandler(int sig);
void APP__Init();
void APP__Deinit();

/*
    main
    Giordano C Moro
    xx/xx/2020
*/
int main()
{
    /* Declarando as variáveis */
	int ret = 0;                /* Verificações de retornos de funções */

    /* Inicializa o Kinect */
    APP__Init();

    /* Loop infinito */
    while ( Semi_Cycles < NUM_OF_SEMI_CYCLES )
    {
        /* "Tira uma foto" */
        //ret = freenect_sync_get_depth((void**)&depth, &timestamp, MY_KINECT_INDEX, FREENECT_DEPTH_11BIT);
        if (ret < 0)
        {
            printf("\nProblem to take picture =(..");
            fflush(stdout);
            APP__Deinit();
            exit(1);
        }
        else
        {
            /* Segue o baile */
        }

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

        printf("\nÂngulo: (%d°) | Direção: (%s)", Device_Angle, (Device_Angle_Direction == KINECT_ANGLE_DIR_UP ? "subindo" : "descendo"));
        fflush(stdout);
        freenect_set_tilt_degs(fn_dvc, Device_Angle);

        /* Espera o KINECT terminar o movimento */
        do
        {
            freenect_update_tilt_state(fn_dvc);
            Device_State = freenect_get_tilt_state(fn_dvc);
        } while ( Device_State->tilt_status == TILT_STATUS_MOVING );
    }

    /* Encerra o Kinect */
    KINECT__Deinit();

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
    int ret = 0;

	/* Inicializa a biblioteca freenect */
	ret = freenect_init(&fn_ctx, NULL);
	if (ret < 0)
	{
        printf("\nfreenect_init failed!");
        fflush(stdout);
        exit(1);
	}
	else
	{
        printf("\nfreenect_init success!");
        fflush(stdout);
	}

	/* Logs na tela para DEBUG, no momento */
    //freenect_set_log_level(fn_ctx, FREENECT_LOG_DEBUG);

    /* Seleciona os sub-dispositivos do KINECT */
	freenect_select_subdevices(fn_ctx, (freenect_device_flags)(FREENECT_DEVICE_MOTOR | FREENECT_DEVICE_CAMERA));

	/* ? */
    ret = freenect_open_device(fn_ctx, &fn_dvc, MY_KINECT_INDEX);

    if ( ret < 0 )
    {
        printf("\ncould not open the device!");
        fflush(stdout);
		freenect_shutdown(fn_ctx);
        exit(1);
    }
    else
    {
        printf("\nopened the device successfully!");
        fflush(stdout);
    }

    /* Sets iniciais */
    freenect_set_tilt_degs(fn_dvc, INITIAL_KINECT_MOTOR_ANGLE);
	freenect_set_led(fn_dvc, LED_GREEN);

    /* Espera o KINECT terminar o movimento */
    do
    {
        freenect_update_tilt_state(fn_dvc);
        Device_State = freenect_get_tilt_state(fn_dvc);
    } while ( Device_State->tilt_status == TILT_STATUS_MOVING );

    /* Inicializa a parte de DEPTH do KINECT */
	freenect_set_depth_callback(fn_dvc, KINECT__Depth_Callback);
	freenect_set_depth_mode(fn_dvc, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT));
	freenect_start_depth(fn_dvc);
}

/*
    KINECT__Depth_Callback
    Giordano C Moro
    30/06/2020
*/
void KINECT__Depth_Callback(freenect_device * dvc, void * v_depth, uint32_t timestamp)
{
    static uint8_t i = 0;
    uint16_t * depth = (uint16_t *)v_depth;

    FILE__Write( depth );

    printf("\nEntrou CALLBACK %u vez(es) | depth: %u", (++i), depth[1]);
    fflush(stdout);
}

/*
    KINECT__Deinit
    Giordano C Moro
    28/06/2020
*/
void KINECT__Deinit()
{
    freenect_stop_depth(fn_dvc);
    freenect_close_device(fn_dvc);
    freenect_shutdown(fn_ctx);
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

    fprintf(fp, "\n%u,%d,%i,%d", index++, *(uint16_t *)data, Device_Angle, Device_Angle_Direction);

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

    /* Alocando memória */
	//depth = calloc(640*480, sizeof(uint8_t));

	/* Faz ajuste para o comando "Ctrl + c" */
	//signal(SIGINT, INThandler);

	/* Inicializa o KINECT */
    KINECT__Init();
}

/*
    APP__Init
    Giordano C Moro
    30/06/2020
*/
void APP__Deinit()
{
    /* Libera o ponteiro */
    //free(depth);

    /* Desinicializa o KINECT */
    KINECT__Deinit();
}
