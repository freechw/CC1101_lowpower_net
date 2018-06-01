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

void RF_send_ACK_to_gateway(INT8U function_byte)
{
    uint8_t status;
    wireless_frame_head(GATEWAY_ADDR,function_byte,RF_FRAME_ACK);
    tx_Base_DATA.size = 5;
    status = RF_send_data_to_gateway(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
    #ifdef RF_PROTOCOL_DEBUG
    if(status == 1)
    {
        uart_send_temp[0] = FRAMEDATA_HEAD;
        uart_send_temp[1] = UART_LOCAL_DEVICE;
        uart_send_temp[2] = UART_FRAME_RETURN_STATUS;  
        uart_send_temp[3] = UART_ACK_SEND_TO_GATEWAY_OK; 
        uart_send_bits(uart_send_temp,4);
    }
    else
    {
        uart_send_temp[0] = FRAMEDATA_HEAD;
        uart_send_temp[1] = UART_LOCAL_DEVICE;
        uart_send_temp[2] = UART_FRAME_RETURN_STATUS;  
        uart_send_temp[3] = UART_ACK_SEND_TO_GATEWAY_FAIL; 
        uart_send_bits(uart_send_temp,4);        
    }
    #endif
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
    #ifdef RF_PROTOCOL_DEBUG
    if(status == 1)
    {
        uart_send_temp[0] = FRAMEDATA_HEAD;
        uart_send_temp[1] = UART_LOCAL_DEVICE;
        uart_send_temp[2] = UART_FRAME_RETURN_STATUS;  
        uart_send_temp[3] = UART_ACK_SEND_TO_TERMINAL_OK; 
        uart_send_bits(uart_send_temp,4);
    }
    else
    {
        uart_send_temp[0] = FRAMEDATA_HEAD;
        uart_send_temp[1] = UART_LOCAL_DEVICE;
        uart_send_temp[2] = UART_FRAME_RETURN_STATUS;  
        uart_send_temp[3] = UART_ACK_SEND_TO_TERMINAL_FAIL; 
        uart_send_bits(uart_send_temp,4);        
    }   
    #endif
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
    if(RF_receive_addr > GATEWAY_ADDR && RF_send_addr == GATEWAY_ADDR)//��Ϊ�ն˽������ص�ACK
    { 
        #ifdef RF_PROTOCOL_DEBUG
        uart_send_temp[0] = FRAMEDATA_HEAD;
        uart_send_temp[1] = RF_send_addr;
        uart_send_temp[2] = UART_FRAME_RETURN_STATUS;  
        uart_send_temp[3] = UART_ACK_GET_FROM_GATEWAY_OK; 
        uart_send_bits(uart_send_temp,4);    
        #endif
        RF_send_ACK_to_gateway(RF_function_byte); //�ն��ٷ���һ��ACK
    }
    else if(RF_receive_addr == GATEWAY_ADDR && RF_send_addr > GATEWAY_ADDR)//��Ϊ���ؽ����ն˵�ACK
    {
        if(RF_function_byte == RF_FRAME_SET_ADDR)//�ն����õ�ַ����ACK
        {
            uart_send_temp[0] = FRAMEDATA_HEAD;
            uart_send_temp[1] = RF_send_addr;
            uart_send_temp[2] = UART_FRAME_RETURN_STATUS;  
            uart_send_temp[3] = UART_RF_SET_TERMINAL_ADDR_OK; 
            uart_send_bits(uart_send_temp,4); 
        }
        else
        {
            #ifdef RF_PROTOCOL_DEBUG        
            uart_send_temp[0] = FRAMEDATA_HEAD;
            uart_send_temp[1] = RF_send_addr;
            uart_send_temp[2] = UART_FRAME_RETURN_STATUS;  
            uart_send_temp[3] = UART_ACK_GET_FROM_TERMINAL_OK; 
            uart_send_bits(uart_send_temp,4);            
            #endif
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
        if(RF_send_addr > GATEWAY_ADDR && RF_receive_addr == GATEWAY_ADDR)//����ACK
        {
            //���Ͷ����նˣ�����������
            RF_send_ACK_to_terminal(RF_send_addr,RF_function_byte);
        }
        else if(RF_send_addr == GATEWAY_ADDR && RF_receive_addr > GATEWAY_ADDR)
        {
            //���Ͷ������أ��������ն�
            RF_send_ACK_to_gateway(RF_function_byte);
        } 
    }
    else if(RF_function_byte == RF_FRAME_SET_ADDR)//���õ�ַ
    {
        if(RF_send_addr == GATEWAY_ADDR && RF_receive_addr > GATEWAY_ADDR)
        {
            //���Ͷ������أ��������ն�
            if(rx_Base_DATA.payload[5] > DEFAULT_TERMINAL_ADDR)//��ַ���������óɹ㲥�����ػ��ն�Ĭ�ϵ�ַ
            {
                Set_Local_ADDR(rx_Base_DATA.payload[5]);
                RF_send_ACK_to_gateway(RF_function_byte);
            }
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
    if(Local_ADDR > GATEWAY_ADDR)//������������
    {
        if(uart_function_byte == UART_FRAME_SET_ADDR) //���õ�ַ
        {
            if(uart_receive_temp[3] > DEFAULT_TERMINAL_ADDR) //��ַ���������óɹ㲥�����ػ��ն�Ĭ�ϵ�ַ
            {
                CC1101SetIdle();
                Set_Local_ADDR(uart_receive_temp[3]);
                CC1101SetSWOR();
                #ifdef RF_PROTOCOL_DEBUG                
                uart_send_temp[0] = FRAMEDATA_HEAD;
                uart_send_temp[1] = UART_LOCAL_DEVICE;
                uart_send_temp[2] = UART_FRAME_SET_ADDR;  
                uart_send_temp[3] = UART_SET_LOCAL_ADDR_OK; 
                uart_send_bits(uart_send_temp,4);                
                #endif
            }
        }
    }
    if(uart_function_byte == UART_FRAME_GET_LOCAL_ADDR)
    {
        uart_send_temp[0] = FRAMEDATA_HEAD;
        uart_send_temp[1] = UART_LOCAL_DEVICE;
        uart_send_temp[2] = UART_FRAME_GET_LOCAL_ADDR;  
        uart_send_temp[3] = Local_ADDR; 
        uart_send_bits(uart_send_temp,4);
    }
}

void uart_wireless_cmd_handle(void)
{
    uint8_t status = 0;
    if(uart_function_byte == UART_FRAME_SET_ADDR)
    {
        if(Local_ADDR == GATEWAY_ADDR && uart_target_addr > GATEWAY_ADDR) //���������ز���Ȩ�����ն˵�ַ
        {
            wireless_frame_head(uart_target_addr,uart_function_byte,RF_FRAME_CMD);
            tx_Base_DATA.payload[5] = uart_receive_temp[3];
            tx_Base_DATA.size = 6;
            status = RF_send_data_to_terminal(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
            #ifdef RF_PROTOCOL_DEBUG 
            if(status != 0)//���ͳɹ�
            {
                uart_send_temp[0] = FRAMEDATA_HEAD;
                uart_send_temp[1] = UART_LOCAL_DEVICE;
                uart_send_temp[2] = UART_FRAME_RETURN_STATUS;  
                uart_send_temp[3] = UART_RF_SEND_TERMINAL_ADDR_OK; 
                uart_send_bits(uart_send_temp,4);
            }
            else
            {
                uart_send_temp[0] = FRAMEDATA_HEAD;
                uart_send_temp[1] = UART_LOCAL_DEVICE;
                uart_send_temp[2] = UART_FRAME_RETURN_STATUS;  
                uart_send_temp[3] = UART_RF_SEND_TERMINAL_ADDR_FAIL; 
                uart_send_bits(uart_send_temp,4);                
            }
            #endif
        }
    }
    else if(uart_function_byte == RF_FRAME_DATA_TRANSFER)
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
        if(uart_target_addr == GATEWAY_ADDR && Local_ADDR > GATEWAY_ADDR)//�������նˣ�����������
        {
            status = RF_send_data_to_gateway(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
        }
        else if(uart_target_addr > GATEWAY_ADDR && Local_ADDR == GATEWAY_ADDR)//���������أ��������ն�
        {
            status = RF_send_data_to_terminal(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
        }
        #ifdef RF_PROTOCOL_DEBUG 
        if(status != 0)//���ͳɹ�
        {
            uart_send_temp[0] = FRAMEDATA_HEAD;
            uart_send_temp[1] = UART_LOCAL_DEVICE;
            uart_send_temp[2] = UART_FRAME_RETURN_STATUS;  
            uart_send_temp[3] = UART_RF_DATA_SEND_OK; 
            uart_send_bits(uart_send_temp,4);
        }
        else
        {
            uart_send_temp[0] = FRAMEDATA_HEAD;
            uart_send_temp[1] = UART_LOCAL_DEVICE;
            uart_send_temp[2] = UART_FRAME_RETURN_STATUS;  
            uart_send_temp[3] = UART_RF_DATA_SEND_FAIL; 
            uart_send_bits(uart_send_temp,4);            
        }
        #endif
    }
}

void uart_protocol_handle(void)
{
    uart_send_bits(uart_receive_temp,uart_receive_num); //�ش�
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


