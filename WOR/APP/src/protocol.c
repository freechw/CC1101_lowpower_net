/* Includes ------------------------------------------------------------------*/
#include "protocol.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
//*******************����0x00Ϊ�㲥��ַ��0x01�̶�Ϊ���ص�ַ*******************//
//************************0x02Ϊ�ն�δ��ʼ����ַ*****************************//
//************************0x03~0xFEΪ�ն˿��õ�ַ����251��*******************//
INT8U Device_addrpool[32]={0};//��ַ�����
INT8U Device_aliveadd[255]={0};//�豸���߳�
/* Private variables ---------------------------------------------------------*/
INT8U RF_ack_get_flag = 0;
INT8U send_addr;
INT8U receive_addr;
INT8U function_byte;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
void frame_head(INT8U target_addr,INT8U function_byte,INT8U cmd_type_byte)
{
    tx_Base_DATA.payload[0] = FRAMEDATA_HEAD;
    tx_Base_DATA.payload[1] = Local_ADDR;
    tx_Base_DATA.payload[2] = target_addr;
    tx_Base_DATA.Target_addr = target_addr;
    tx_Base_DATA.payload[3] = function_byte;      
    tx_Base_DATA.payload[4] = cmd_type_byte;
}

void wireless_addr_mark(INT8U addr)
{
    INT8U byte_index = addr / 8;
    INT8U bit_index = addr % 8;
    Device_addrpool[byte_index] |= (0x01 << bit_index);//��ַռ�ñ��
    Device_aliveadd[addr]++;
    if(Device_aliveadd[addr] >= 3)
    {
        Device_aliveadd[addr] = 3;
    }
}

void wireless_addr_getsync(INT8U *sync_addrdata)
{
    INT8U i,j,add = 0;
    for(i=0;i<32;i++)
    {
        for(j=0;j<8;j++)
        {
            if((sync_addrdata[i] & (0x01<<j)) != 0)
            {
                if((Device_addrpool[i] & (0x01<<j)) == 0)//�������豸
                {
                    Device_addrpool[i] |= (0x01 << j);
                    add = 1;
                }
            }
        }
    }
    if(add == 1)//���浽EEPROM��  
    {
        SaveE2PDatas(2, Device_addrpool, 32);
    }
}

void wireless_addr_demark(INT8U addr)
{
    if(Device_aliveadd[addr] > 0)
    {
        Device_aliveadd[addr]--;
    }
}

INT8U allocate_terminal_addr(void)
{
    INT8U i,j,temp=0;
    for(i=0;i<32;i++)
    {
        for(j=0;j<8;j++)
        {
            if((Device_addrpool[i] & (0x01<<j)) == 0)
            {
                temp = i*8 + j;
                break;
            }
        }
        if(temp != 0)
        {
            break;
        }
    }    
    return temp;
}

uint8_t RF_terminal_send_data(INT8U *txbuffer, INT8U size, INT8U addr)
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
        if(status != 1)
        {
            delay_ms(10);//10ms
        }
    }
    RF_check_timer = 0;
    return status;
}

void RF_terminal_send_ACK(INT8U function_byte)
{
    uint8_t status;
    frame_head(TYPE_GATEWAY,function_byte,FRAME_ACK);
    tx_Base_DATA.size = 5;
    status = RF_terminal_send_data(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
    if(status == 1)
    {
        uart_send_bits("TCK_S\r\n",7);
    }
}

uint8_t RF_gateway_send_data(INT8U *txbuffer, INT8U size, INT8U addr)
{
    uint8_t status = 0;
    RF_check_ack_timer = 0;
    while(RF_ack_get_flag != 1)
    {
        status = RF_terminal_send_data(txbuffer, size, addr);
        if(RF_check_ack_timer >= 10000) //2s
        {
            status = 0;
            break;
        }
        if(RF_ack_get_flag != 1)
        {
            delay_ms(50);//50ms
        }
    }
    RF_ack_get_flag = 0;
    return status;
}

void RF_gateway_send_ACK(INT8U target_addr,INT8U function_byte)
{
    uint8_t status;
    frame_head(target_addr,function_byte,FRAME_ACK);
    tx_Base_DATA.size = 5;
    //uart_send_bits(tx_Base_DATA.payload,tx_Base_DATA.size);   
    status = RF_gateway_send_data(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
    if(status == 1)
    {
        uart_send_bits("GCK_S\r\n",7);
    }  
    else
    {
        wireless_addr_demark(tx_Base_DATA.Target_addr);
    }
}

void RF_gateway_send_terminal_allocate_addr(INT8U target_addr)
{
    INT8U status = 0;
    INT8U allocate_addr = allocate_terminal_addr();
    frame_head(target_addr,FRAME_APPLY_TERMINAL_ADDR,FRAME_ACK);
    tx_Base_DATA.payload[5] = allocate_addr;
    tx_Base_DATA.size = 6;
    status = RF_gateway_send_data(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
    if(status == 1)
    {
        uart_send_bits("GSD_S\r\n",7);
    }
}
void RF_terminal_apply_addr(void)
{
    INT8U status = 0;
    frame_head(TYPE_GATEWAY,FRAME_APPLY_TERMINAL_ADDR,FRAME_CMD);
    tx_Base_DATA.size = 5;
    status = RF_terminal_send_data(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
    if(status == 1)
    {
        uart_send_bits("TAD_S\r\n",7);
    }
}

void RF_get_ack(void)
{
    if(rx_Base_DATA.payload[0] == FRAMEDATA_HEAD)
    {
        send_addr = rx_Base_DATA.payload[1];
        wireless_addr_mark(send_addr);
        if(rx_Base_DATA.payload[4] == FRAME_ACK)
        {
            RF_ack_get_flag = 1;
        }
    }
}

void wireless_addr_sendsync()
{
    INT8U status = 0;
    frame_head(TYPE_GATEWAY,FRAME_SYNC_TERMINAL_ADDRPOOL,FRAME_CMD);
    copy_datas(&tx_Base_DATA.payload[5],Device_addrpool,32);
    tx_Base_DATA.size = 37;
    status = RF_terminal_send_data(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
    if(status == 1)
    {
        uart_send_bits("SYNC_S\r\n",7);
    }
}

void recive_ack_handle(void)
{
    uart_send_bits("ACK_G\r\n",7);
    if(receive_addr != TYPE_GATEWAY)//��Ϊ�ն˽��ܵ�ACK
    { 
        if(function_byte == FRAME_APPLY_TERMINAL_ADDR)//�����ն�����ĵ�ַ
        {
            Local_ADDR = rx_Base_DATA.payload[5];
            Set_Local_ADDR(Local_ADDR);
            CC1101SetAddress(Local_ADDR, BROAD_0);    
        }
        RF_terminal_send_ACK(function_byte); //�ն��ٷ���һ��ACK
    }    
}

void recive_cmd_handle(void)
{
    if(function_byte == FRAME_DATA_TRANSFER)//���ݴ���
    {
        if(send_addr != TYPE_GATEWAY)//����ACK
        {
            //���Ͷ����ն�
            RF_gateway_send_ACK(send_addr,function_byte);
        }
        else
        {
            RF_terminal_send_ACK(function_byte);
        } 
        uart_send_bits(&rx_Base_DATA.payload[5],rx_Base_DATA.size - 5); //���͵�����  
    }
    else if(function_byte == FRAME_APPLY_TERMINAL_ADDR)//�ն������������ַ
    {
        RF_gateway_send_terminal_allocate_addr(send_addr);//Ҳ��һ��ACK
    }
    else if(function_byte == FRAME_SYNC_TERMINAL_ADDRPOOL)
    {
        uart_send_bits("SYNC_G\r\n",7);
        wireless_addr_getsync(&rx_Base_DATA.payload[5]);
    }
}

void wireless_protocol_handle(void)
{
    if(rx_Base_DATA.payload[0] == FRAMEDATA_HEAD)
    {
        send_addr = rx_Base_DATA.payload[1];
        receive_addr = rx_Base_DATA.payload[2];
        function_byte = rx_Base_DATA.payload[3];
        if(rx_Base_DATA.payload[4] == FRAME_ACK)//��ΪACK
        {
            recive_ack_handle();
        }
        else //��ΪCMD
        {
            recive_cmd_handle();    
        }
    }
}

void uart_protocol_handle(void)
{
    uint8_t status = 0;
    uart_send_bits(uart_receive_temp,uart_receive_num); //�ش�
    if(Local_Device_Type == TYPE_TERMINAL)//�ն˴���
    {
        status = RF_terminal_send_data(uart_receive_temp, uart_receive_num, TYPE_GATEWAY);
    }
    else//���ش���
    {
        frame_head(uart_receive_temp[0],FRAME_DATA_TRANSFER,FRAME_CMD);
        if(uart_receive_num - 1 < 55)//��ȴuart_receive_temp[0]�ķ��͵�ַ�����һ�η���55���ֽ����ݡ�
        {
            copy_datas(&tx_Base_DATA.payload[5],&uart_receive_temp[1],uart_receive_num - 1);
            tx_Base_DATA.size = uart_receive_num + 4; //uart_receive_num - 1 + 5
        }
        else
        {
            copy_datas(&tx_Base_DATA.payload[5],&uart_receive_temp[1],55);
            tx_Base_DATA.size = 60;
        }
        status = RF_gateway_send_data(tx_Base_DATA.payload, tx_Base_DATA.size, tx_Base_DATA.Target_addr);
    }
    if(status == 0)//���Ͳ��ɹ�
    {
        wireless_addr_demark(Target_ADDR);
    }
    else
    {
        uart_send_bits("Data_S\r\n",8);
    }
}


