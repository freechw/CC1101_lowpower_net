/* Includes ------------------------------------------------------------------*/
#include "protocol.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
//*******************����0x00Ϊ�㲥��ַ��0x01�̶�Ϊ���ص�ַ*******************//
//************************0x02Ϊ�ն�δ��ʼ����ַ*****************************//
//************************0x03~0xFEΪ�ն˿��õ�ַ����251��*******************//
/* Private variables ---------------------------------------------------------*/
INT8U RF_ack_get_flag = 0;

INT8U RF_send_addr = 0;
INT8U RF_receive_addr = 0;
INT8U RF_function_byte = 0;

INT8U uart_target_addr = 0;
INT8U uart_function_byte = 0;
INT8U uart_protocol_debug = 0;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
void wireless_frame_head(INT8U target_addr,INT8U function_byte,INT8U cmd_type_byte)
{
    tx_Base_DATA.payload[0] = FRAMEDATA_HEAD;
    tx_Base_DATA.payload[1] = Local_ADDR;
    tx_Base_DATA.payload[2] = target_addr;
    tx_Base_DATA.Target_addr = target_addr;
    tx_Base_DATA.payload[3] = function_byte;      
    tx_Base_DATA.payload[4] = cmd_type_byte;
}

void uart_return(INT8U target_addr,INT8U function_byte,INT8U state_byte)
{
    uart_send_temp[0] = FRAMEDATA_HEAD;
    uart_send_temp[1] = target_addr;
    uart_send_temp[2] = function_byte;  
    uart_send_temp[3] = state_byte; 
    uart_send_bits(uart_send_temp,4);
}

uint8_t RF_send_data_to_gateway(INT8U *txbuffer, INT8U size, INT8U addr)
{
    uint8_t status = 0;
    RF_check_timer = 0;
    while(status != 1)
    {
        status = RF_TX_DATA(txbuffer, size, addr); 
        if(RF_check_timer >= 5000) //1s
        {
            break;
        }
        if(status != 1)   //������ʧ�ܣ��ز���ײ��������ʱ10ms�ٷ��ͣ������ʱ1s
        {
            delay_ms(10);//10ms
        }
    }
    RF_check_timer = 0;
    return status;
}

void RF_send_ACK_to_gateway(INT8U target_addr,INT8U function_byte)
{
    uint8_t status;
    wireless_frame_head(target_addr,function_byte,RF_FRAME_ACK);
    tx_Base_DATA.size = 5;
    status = RF_send_data_to_gateway(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
    if(uart_protocol_debug)
    {
        if(status == 1)
        {
            uart_return(UART_LOCAL_DEVICE,UART_FRAME_RETURN_STATUS,UART_ACK_SEND_TO_GATEWAY_OK);
        }
        else
        { 
            uart_return(UART_LOCAL_DEVICE,UART_FRAME_RETURN_STATUS,UART_ACK_SEND_TO_GATEWAY_FAIL);
        }
    }
}

uint8_t RF_send_data_to_terminal(INT8U *txbuffer, INT8U size, INT8U addr)
{
    uint8_t status = 0;
    RF_check_ack_timer = 0;
    while(RF_ack_get_flag != 1)
    {
        status = RF_send_data_to_gateway(txbuffer, size, addr);
        if(RF_check_ack_timer >= 10000) //2s
        {
            status = 0;
            break;
        }
        if(RF_ack_get_flag != 1) //�ȴ��ն˵�ACKӦ��
        {
            delay_ms(50);//50ms
        }
    }
    RF_ack_get_flag = 0;
    return status;
}

void RF_send_ACK_to_terminal(INT8U target_addr,INT8U function_byte)
{
    uint8_t status;
    wireless_frame_head(target_addr,function_byte,RF_FRAME_ACK);
    tx_Base_DATA.size = 5;
    status = RF_send_data_to_terminal(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
    if(uart_protocol_debug)
    {
        if(status == 1)
        {
            uart_return(UART_LOCAL_DEVICE,UART_FRAME_RETURN_STATUS,UART_ACK_SEND_TO_TERMINAL_OK);
        }
        else
        {
            uart_return(UART_LOCAL_DEVICE,UART_FRAME_RETURN_STATUS,UART_ACK_SEND_TO_TERMINAL_FAIL);
        }   
    }
}

void RF_get_ack(void)
{
    if(rx_Base_DATA.payload[0] == FRAMEDATA_HEAD)
    {
        if(rx_Base_DATA.payload[4] == RF_FRAME_ACK)
        {
            RF_ack_get_flag = 1;
        }
    }
}

void RF_recive_ack_handle(void)
{
    if(RF_receive_addr >= DEFAULT_TERMINAL_ADDR && RF_send_addr < DEFAULT_TERMINAL_ADDR)//��Ϊ�ն˽������ص�ACK
    { 
        uart_return(RF_send_addr,UART_FRAME_RETURN_STATUS,UART_ACK_GET_FROM_GATEWAY_OK);
        RF_send_ACK_to_gateway(RF_send_addr,RF_function_byte); //�ն��ٷ���һ��ACK
    }
    else if(RF_receive_addr < DEFAULT_TERMINAL_ADDR)//��Ϊ���ؽ���ACK
    {
        if(RF_function_byte == RF_FRAME_SET_ADDR)//�ն����õ�ַ����ACK
        {
            uart_return(RF_send_addr,UART_FRAME_RETURN_STATUS,UART_RF_SET_TERMINAL_ADDR_OK);
        }
        else if(RF_function_byte == RF_FRAME_GET_TARGET_RSSI)
        {
            uart_return(RF_send_addr,UART_FRAME_GET_TARGET_RSSI,rx_Base_DATA.RSSI);
        }
        else
        { 
            uart_return(RF_send_addr,UART_FRAME_RETURN_STATUS,UART_ACK_GET_FROM_TERMINAL_OK);
        }
    }
}

void RF_recive_cmd_handle(void)
{
    if(RF_function_byte == RF_FRAME_DATA_TRANSFER)//���ݴ���
    {
        uart_send_temp[0] = FRAMEDATA_HEAD;
        uart_send_temp[1] = RF_send_addr;
        uart_send_temp[2] = UART_FRAME_DATA_TRANSFER;  
        uart_send_temp[3] = rx_Base_DATA.size - 5; 
        copy_datas(&uart_send_temp[4],&rx_Base_DATA.payload[5],rx_Base_DATA.size - 5);
        uart_send_bits(uart_send_temp,rx_Base_DATA.size - 1); //���͵����� 
        if(RF_send_addr >= DEFAULT_TERMINAL_ADDR)//����ACK
        {
            //���Ͷ����ն�
            RF_send_ACK_to_terminal(RF_send_addr,RF_function_byte);
        }
        else if(RF_send_addr < DEFAULT_TERMINAL_ADDR)
        {
            //���Ͷ�������
            RF_send_ACK_to_gateway(RF_send_addr,RF_function_byte);
        } 
    }
    else if(RF_function_byte == RF_FRAME_SET_ADDR)//���õ�ַ
    {
        if(RF_send_addr < DEFAULT_TERMINAL_ADDR && RF_receive_addr >= DEFAULT_TERMINAL_ADDR)
        {
            //���Ͷ������أ��������ն�
            if(rx_Base_DATA.payload[5] > DEFAULT_TERMINAL_ADDR)//��ַ���������óɹ㲥�����ػ��ն�Ĭ�ϵ�ַ
            {
                Set_Local_ADDR(rx_Base_DATA.payload[5]);
                RF_send_ACK_to_gateway(RF_send_addr,RF_function_byte);
            }
        }
    }
    else if(RF_function_byte == RF_FRAME_GET_TARGET_RSSI)//��ȡRSSI�ź�ǿ��
    {
        //���Ͷ�������
        if(RF_send_addr < DEFAULT_TERMINAL_ADDR)
        {
            RF_send_ACK_to_gateway(RF_send_addr,RF_function_byte);
        } 
    }
}

void wireless_protocol_handle(void)
{
    if(rx_Base_DATA.payload[0] == FRAMEDATA_HEAD)
    {
        RF_send_addr = rx_Base_DATA.payload[1];
        RF_receive_addr = rx_Base_DATA.payload[2];
        RF_function_byte = rx_Base_DATA.payload[3];
        if(rx_Base_DATA.payload[4] == RF_FRAME_ACK)//��ΪACK
        {
            RF_recive_ack_handle();
        }
        else if(rx_Base_DATA.payload[4] == RF_FRAME_CMD)//��ΪCMD
        {
            RF_recive_cmd_handle();    
        }
    }
}

void uart_local_cmd_handle(void)
{
    if(uart_function_byte == UART_FRAME_SET_ADDR) //���õ�ַ
    {
        if(Local_ADDR < DEFAULT_TERMINAL_ADDR)//����Ϊ����
        {
            if((uart_receive_temp[3] > DEFAULT_GATEWAY_ADDR)&&(uart_receive_temp[3] < DEFAULT_TERMINAL_ADDR)) //��ַ���������óɹ㲥��Ĭ�����ػ��ն˵�ַ
            {
                Set_Local_ADDR(uart_receive_temp[3]);                       
                uart_return(UART_LOCAL_DEVICE,UART_FRAME_RETURN_STATUS,UART_SET_LOCAL_ADDR_OK);
            }
            else
            {    
                uart_return(UART_LOCAL_DEVICE,UART_FRAME_RETURN_STATUS,UART_SET_FORBIDDEN_ADDR);
            }
        }
        else if(Local_ADDR >= DEFAULT_TERMINAL_ADDR)//����Ϊ�ն�
        {      
            if(uart_receive_temp[3] > DEFAULT_TERMINAL_ADDR) //��ַ���������óɹ㲥�����ػ�Ĭ���ն˵�ַ
            {
                CC1101SetIdle();
                Set_Local_ADDR(uart_receive_temp[3]);
                CC1101SetSWOR();                         
                uart_return(UART_LOCAL_DEVICE,UART_FRAME_RETURN_STATUS,UART_SET_LOCAL_ADDR_OK);
            }
            else
            {    
                uart_return(UART_LOCAL_DEVICE,UART_FRAME_RETURN_STATUS,UART_SET_FORBIDDEN_ADDR);
            }
        }
    }
    else if(uart_function_byte == UART_FRAME_GET_LOCAL_ADDR)//��ȡ��ַ
    {
        uart_return(UART_LOCAL_DEVICE,UART_FRAME_GET_LOCAL_ADDR,Local_ADDR);
    }
    else if(uart_function_byte == UART_FRAME_DEBUG_SELECT)//DEBUG����
    {
        uart_protocol_debug = uart_receive_temp[3];
    }
}

void uart_wireless_cmd_handle(void)
{
    uint8_t status = 0;
    if(uart_function_byte == UART_FRAME_SET_ADDR)
    {
        if(Local_ADDR < DEFAULT_TERMINAL_ADDR && uart_target_addr >= DEFAULT_TERMINAL_ADDR) //���������ز���Ȩ���������ն˵�ַ
        {
            if(uart_receive_temp[3] > DEFAULT_TERMINAL_ADDR)//��ַ���������óɹ㲥�����ػ��ն�Ĭ�ϵ�ַ
            {
                wireless_frame_head(uart_target_addr,RF_FRAME_SET_ADDR,RF_FRAME_CMD);
                tx_Base_DATA.payload[5] = uart_receive_temp[3];
                tx_Base_DATA.size = 6;
                status = RF_send_data_to_terminal(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
            }
            else
            {
                uart_return(UART_LOCAL_DEVICE,UART_FRAME_RETURN_STATUS,UART_SET_FORBIDDEN_ADDR);
            }
        }
        else
        {      
            uart_return(UART_LOCAL_DEVICE,UART_FRAME_RETURN_STATUS,UART_CMD_PERMISION_DENIED);
        }
    }
    else if(uart_function_byte == UART_FRAME_GET_TARGET_RSSI)
    {
        if(Local_ADDR < DEFAULT_TERMINAL_ADDR) //���������ز���Ȩ��ȡRSSI
        {
            wireless_frame_head(uart_target_addr,RF_FRAME_GET_TARGET_RSSI,RF_FRAME_CMD);
            tx_Base_DATA.size = 5;
            if(uart_target_addr >= DEFAULT_TERMINAL_ADDR)//Ŀ��Ϊ�ն�
            {
                status = RF_send_data_to_terminal(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
            }
            else //Ŀ��Ϊ����
            {
                status = RF_send_data_to_gateway(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
            }
        }
        else
        {
            uart_return(UART_LOCAL_DEVICE,UART_FRAME_RETURN_STATUS,UART_CMD_PERMISION_DENIED);
        }
    }
    else if(uart_function_byte == UART_FRAME_DATA_TRANSFER)
    {
        wireless_frame_head(uart_target_addr,uart_function_byte,RF_FRAME_CMD);
        if(uart_receive_temp[3] <= 55)//���ݳ���
        {
            copy_datas(&tx_Base_DATA.payload[5],&uart_receive_temp[4],uart_receive_temp[3]);
            tx_Base_DATA.size = uart_receive_temp[3] + 5; 
        }
        else
        {
            copy_datas(&tx_Base_DATA.payload[5],&uart_receive_temp[4],55);
            tx_Base_DATA.size = 60;             
        }
        if(uart_target_addr < DEFAULT_TERMINAL_ADDR)//����������
        {
            status = RF_send_data_to_gateway(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
        }
        else if(uart_target_addr >= DEFAULT_TERMINAL_ADDR)//�������ն�
        {
            status = RF_send_data_to_terminal(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
        }
    }
    if(uart_protocol_debug)
    {
        if(status != 0)//���ͳɹ�
        {
            uart_return(UART_LOCAL_DEVICE,UART_FRAME_RETURN_STATUS,UART_RF_SEND_OK);
        }
        else
        {        
            uart_return(UART_LOCAL_DEVICE,UART_FRAME_RETURN_STATUS,UART_RF_SEND_FAIL);
        }
    }
}

void uart_protocol_handle(void)
{
    if(uart_protocol_debug)
    {
        uart_send_bits(uart_receive_temp,uart_receive_num); //�ش�
    }
    if(uart_receive_temp[0] == FRAMEDATA_HEAD)
    {
        uart_target_addr = uart_receive_temp[1];
        uart_function_byte = uart_receive_temp[2];
        if(uart_target_addr == 0)//��Ϊ����ָ��
        {
            uart_local_cmd_handle();
        }
        else //��Ϊ����ָ��
        {
            uart_wireless_cmd_handle();
        }
    }
}


