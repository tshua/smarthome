#include "protocol.h"
#include <string.h>
#include <stdint.h>

Protocol::Protocol()
{
	torken = NULL;
	data = NULL;
}

Protocol::Protocol(Protocol &p)
{
	this->package_header    = p.package_header;
	this->len_low           = p.len_low;
	this->len_high          = p.len_high;
	this->cmd_type          = p.cmd_type;
	this->cmd               = p.cmd;
	this->cmd_order_low     = p.cmd_order_low;
	this->cmd_order_high    = p.cmd_order_high;
	this->have_extend       = p.have_extend;

	memcpy(this->extend_info, p.extend_info, 2);

	this->status            = p.status;

	memcpy(this->device_id, p.device_id, 8);

	this->torken_len        = p.torken_len;

	this->torken            = new unsigned char[this->torken_len];
	memcpy(this->torken, p.torken, p.torken_len);

	this->data              = new unsigned char[this->len_low + this->len_high*256 - PACKAGE_LEN_EXCEPT_DATA - this->torken_len];
	memcpy(this->data, p.data, this->len_low + this->len_high*256 - PACKAGE_LEN_EXCEPT_DATA - this->torken_len);

	this->have_CRC16    = p.have_CRC16;
	this->CRC_low       = p.CRC_low;
	this->CRC_high      = p.CRC_high;
	this->package_tail  = p.package_tail;

}

Protocol::~Protocol()
{

	if(torken != NULL)
		delete [] torken;
	if(data != NULL)
		delete [] data;


}
void Protocol::clean_data()
{
	this->package_header    = 0;
	this->len_low           = 0; 
	this->len_high          = 0;
	this->cmd_type          = 0;
	this->cmd               = 0;
	this->cmd_order_low     = 0;
	this->cmd_order_high    = 0;
	this->have_extend       = 0;

	memset(this->extend_info, 0, 2); 

	this->status            = 0;

	memset(this->device_id, 0, 8); 

	this->torken_len        = 0; 

	this->torken            = NULL;

	this->data              = NULL;

	this->have_CRC16    = 0;
	this->CRC_low       = 0;
	this->CRC_high      = 0;
	this->package_tail  = 0;

}


int Protocol::CRC_16(unsigned char* buf)                      //计算CRC，并填充到package中
{

	int16_t shift,data = 0,val;
	int i, length = len_high*256 + len_low - 4;  //由“包头”至“数据”的所有字节的CRC16运算值; 即: 包头,长度:低位,长度:高位,命令类别,命令字,用户ID,数据.

	shift = CRC_SEED;


	for(i=0;i<length;i++) {
		if((i % 8) == 0)
			data = (*buf++)<<8;
		val = shift ^ data;
		shift = shift<<1;
		data = data <<1;
		if(val&0x8000)
			shift = shift ^ POLY16;
	}

	if(have_CRC16)
	{
		if( CRC_high == (unsigned char)shift &&
				CRC_low  == (unsigned char)(shift>>8) )
			return 0;
		return -1;        //校验失败，数据存在错误。
	}

	CRC_high = (unsigned char)shift;      //添加校验
	CRC_low  = (unsigned char)(shift>>8);
	have_CRC16 = 1;

	return 0;

}

int Protocol::fill_buf(unsigned char* buf)  //填充buf,返回长度
{

	buf[0] = this->package_header;
	buf[1] = this->len_low;
	buf[2] = this->len_high;
	buf[3] = this->cmd_type;
	buf[4] = this->cmd;
	buf[5] = this->cmd_order_low;
	buf[6] = this->cmd_order_high;
	buf[7] = this->have_extend;

	memcpy(&(buf[8]), this->extend_info, 2);

	buf[10] = this->status;

	memcpy(&(buf[11]), this->device_id, 8);

	buf[19] = this->torken_len;

	memcpy(&(buf[20]), this->torken, this->torken_len);

	memcpy(buf + 20 + this->torken_len, this->data, this->len_low + this->len_high*256 - PACKAGE_LEN_EXCEPT_DATA - this->torken_len);

	int pos = 20 + this->torken_len + this->len_low + this->len_high*256 - PACKAGE_LEN_EXCEPT_DATA - this->torken_len;//当前buf的位置
	buf[pos] = this->have_CRC16;
	buf[pos + 1] = this->CRC_low;
	buf[pos + 2] = this->CRC_high;
	buf[pos + 3] = this->package_tail;

	return pos + 4;//返回package长度

}

int Protocol::parse_buf(unsigned char* buf) //解析buf，将buf中数据转换成数据包的形式
{
	this->package_header = buf[0];
	this->len_low        = buf[1];
	this->len_high       = buf[2];
	this->cmd_type       = buf[3];
	this->cmd            = buf[4];
	this->cmd_order_low  = buf[5];
	this->cmd_order_high = buf[6];
	this->have_extend    = buf[7];

	memcpy(this->extend_info, &(buf[8]), 2);

	this->status = buf[10];

	memcpy(this->device_id, &(buf[11]), 8);

	this->torken_len = buf[19];

	this->torken = new unsigned char[this->torken_len];
	memcpy(this->torken, &(buf[20]), this->torken_len);

	this->data = new unsigned char[this->len_low + this->len_high*256 - PACKAGE_LEN_EXCEPT_DATA];
	memcpy(this->data, buf + 20 + this->torken_len, this->len_low + this->len_high*256 - PACKAGE_LEN_EXCEPT_DATA - this->torken_len);

	int pos = 20 + this->torken_len + this->len_low + this->len_high*256 - PACKAGE_LEN_EXCEPT_DATA - this->torken_len;//当前buf的位置
	this->have_CRC16 = buf[pos];
	this->CRC_low    = buf[pos + 1];
	this->CRC_high   = buf[pos + 2];
	this->package_tail = buf[pos + 3];

	return 0;
}

int Protocol::transferEncode(unsigned char* buf)   //添加转义字符
{
	int i ,j, length = buf[1] + buf[2] * 256;
	for(i = 0; i < length; i++)
	{
		if( i == 2 || i == 3 ) //长度固定字节  不转义
			continue;
		if(buf[i] == 0x55)
		{
			j = length;
			while(j != i)
			{
				buf[j] = buf[j-1];
				j--;
			}
			buf[i] = 0x54;
			buf[i+1] = 0x01;
			length++;       //长度加一
			continue;
		}

		if(buf[i] == 0x54)
		{
			j = length;
			while(j != i)
			{
				buf[j] = buf[j-1];
				j--;
			}
			buf[i] = 0x54;
			buf[i+1] = 0x02;
			length++;
			continue;
		}
	}

	return length;
}

int Protocol::transferDecode(unsigned char* buf, int length)   //去除转义字符
{
	int i ,j;
	for(i = 0; i < length; i++)
	{
		if(i == 1 || i == 2)
			continue;
		if( buf[i] == 0x54 && buf[i+1] == 0x01 )
		{
			buf[i] = 0x55;
			j = i+1;
			while(j < length)
			{
				buf[j] = buf[j+1];
				j++;
			}
			length--;       //长度减一
			continue;
		}

		if(buf[i] == 0x54 && buf[i+1] == 0x02 )
		{
			j = i;
			buf[j] = 0x54;
			j++;
			while(j < length)
			{
				buf[j] = buf[j+1];
				j++;
			}
			length--;
			continue;
		}
	}

	return length;
}

