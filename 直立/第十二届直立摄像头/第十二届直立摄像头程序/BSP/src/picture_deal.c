#include "picture_deal.h"

/**************picture_deal_global_var******/
position car_position=normal;	//初始化车在环岛位子标志为normal
ten_status car_ten_status=normal_ten;//初始化车在十字弯位子标志为normal_ten
status   car_status=running;	//车启停状态标志为running
int16 left_line[120]={0};	//左边线数组
int16 right_line[120]={0};	//右边线数组
int16 center_line[120]={0};	//中心线数组（一般情况下中心线与搜线起始点一样，在补线线时不一样）
int16 start_line[120]={0};	//搜线起始点数组
int16 compensation_line[10]={69,69,69,69,70,70,70,71,71,71};//确定搜线起始点的近处十行，若没有边线用此补线
int16 near_lost_count_left=0;	//近处十行左边丢线计数
int16 near_lost_count_righ=0;	//近处十行右边丢线计数
int16 lost_to_have_flag=0;		//丢线找回标志
int16 turn_flag=0;				//转向标志
int16 lost_count=0;				//丢线计数
int16 cp_line[120]={30,32,32,32,34,34,36,36,36,38,38,38,40,40,42,42,42,44,44,44,46,46,48,48,48,50,50,\
52,52,52,54,54,54,56,56,58,58,58,60,60,60,62,62,64,64,64,66,66,68,68,68,70,70,70,\
72,72,74,74,74,76,76,76,78,78,80,80,80,82,82,82,84,84,86,86,86,88,88,90,90,90,\
92,92,92,94,94,96,96,96,98,98,98,100,100,102,102,102,104,104,106,106,106,108,108,\
  108,110,110,112,112,112,114,114,114,116,116,118,118,118,120,120,122};	//环岛补线，进入环岛只能看到一边线，另一边线按照远近比例平移得到。

/**************function*********************/
/**
 * @file		处理图像sline到eline，并得到方向偏差
 *				例如传递进来sline=89，eline=1。
 * @date		2017-4 
 */
float get_picture_diff(int16 sline,int16 eline)
{
  param_reset();
  car_status=check_end_line(sline,(sline-4));//用89-85行检测起跑线
  
  int16 start;
  start=search_start(sline-5,sline-14);		//用84-75行来确定剩下行的搜线起始点
  
  SL(sline-14)=start;						//75行中心线=搜线起始点
  sline=sline-15;							//从74行开始搜线
  
  static int16 final_end;
  final_end=judge_next_line(sline,eline,start);//解算74-1行（得出左右边线和start_line）
  
  judge_position();							//判断车的位置和状态
  
  if(car_position==bef_in_circle_left||car_position==mid_in_circle_left||car_position==aft_in_circle_left||car_position==mid_out_circle_left){
    int16 i;
    for(i=sline;i>=final_end;i--){
      RL(i)=LL(i)+cp_line[i]+24;	//如果在环岛，右线由左线平移而来。（这里+24，越小越快入弯）
      judge_column(i);				//补线之后需要重新计算中心线
    }
  }
	//74-1行的start_line计算偏差用于转向
  float a,b,c;
  a=get_average_diff(final_end,final_end+(75-final_end)/3,start_line);						//图像远处1/3平均偏差
  b=get_average_diff(final_end+(75-final_end)/3+1,final_end+2*(75-final_end)/3,start_line);	//图像中部1/3平均偏差
  c=get_average_diff(final_end+2*(75-final_end)/3+1,74,start_line);  						//图像近处1/3平均偏差
  
  return 0.1*a+0.5*b+0.4*c-80;//返回三部分图像加权偏差
}

/**************function*********************/
/**
 * @file		参数初始化
 * @date		2017-4 
 */
void param_reset(void)
{
  static int16 i;
  for(i=0;i<=119;i++){
    LL(i)=0;	//左边线初始化
    RL(i)=159;	//右边线初始化
    CL(i)=80;	//中心线初始化
    SL(i)=80;	//搜线起始点初始化
  }  
  
  near_lost_count_left=0;
  near_lost_count_righ=0;
  
  lost_to_have_flag=0;
  lost_count=0;
}

/**************function*********************/
/**
 * @file		检测起跑线（用于停车）
 * @date		2017-4 
 */
status check_end_line(int16 sline,int16 eline)
{
  //此处添加判断条件
  
  return running;//还没加停车程序，这里直接返回running
}

/**************function*********************/
/**
 * @file		使用近处sline行与eline行来确定以后的搜线起始点
 * @date		2017-4 
 */
int16 search_start(int16 sline,int16 eline)
{
  static int16 ss_line,start;
  ss_line=sline;
  
  for(;sline>=eline;sline--){
    find_line(sline,80);		//从80到sline行寻线
    judge_init_start(sline);	//
  }
  
  start=(int16)get_average_diff(eline,ss_line,start_line);
  
  if(near_lost_count_left!=0&&near_lost_count_righ!=0){	
    start=80;
  }
  else{
    start=(start>=120?120:start);                   //起始点限幅 
    start=(start<=40?40:start);      
  }  
  
  return start;
}

//start>end  return left_column
/**************function*********************/
/**
 * @file		8点范围内高效率寻线
 * @date		2017-4 
 */
int16 gb_recursion(int16 start,int16 end,int16 line)
{
  if(abs(start-end)==1){
    return end;
  }
  if(PP(line,((start+end)/2)) == BLACK_P){
    return gb_recursion(start,((start+end)/2),line);	//递归
  }
  if(PP(line,((start+end)/2)) == WHITE_P){
    return gb_recursion(((start+end)/2),end,line);		//递归
  }
  return 0;
}

/**************function*********************/
/**
 * @file		start与line之间寻线。8点间隔寻线，找出大致范围，再用高效寻线找具体点
 * @date		2017-4 
 */
void find_line(int16 line,int16 start)
{
  static int16 i;  
  //left
  for(i=start;i>8;i-=8){
    if(PP(line,i) == BLACK_P){
      LL(line)=gb_recursion((i+7),i,line);
      break;
    }
  }  
  if(i<=8){
    LL(line)=gb_recursion((i+8),0,line);
  }
  
  //right
  for(i=start;i<151;i+=8){
    if(PP(line,i)== BLACK_P ){
      RL(line)=gb_recursion((i-7),i,line);
      break;
    }
  }  
  if(i>=151){
    RL(line)=gb_recursion((i-8),159,line);
  }
      
}

/**************function*********************/
/**
 * @file		
 * @date		2017-4 
 */
void judge_init_start(int16 line)
{
  if(LL(line)!=0&&RL(line)!=159){
    SL(line)=(LL(line)+RL(line))/2;
  }
  if(LL(line)!=0&&RL(line)==159){
    SL(line)=LL(line)+compensation_line[line-75];
    near_lost_count_righ++;
  }
  if(LL(line)==0&&RL(line)!=159){
    SL(line)=RL(line)-compensation_line[line-75];
    near_lost_count_left++;
  }
  if(LL(line)==0&&RL(line)==159){
    near_lost_count_righ++;
    near_lost_count_left++;
    SL(line)=80;
  }
  
  SL(line)=(SL(line)>=159?159:SL(line));
  SL(line)=(SL(line)<=  0?  0:SL(line));
}

/**************function*********************/
/**
 * @file		得到数组arry的i到j行的平均偏差
 * @date		2017-4 
 */
float get_average_diff(int16 i,int16 j,int16* arry)
{
  static int16 a;
  static int32 sum;
  sum=0;

  
  quick_sort(arry,i,j);		//快速排序
  for(a=i+1;a<=j-1;a++)
  {
    sum+=arry[a];
  }
  
  return (sum/(float)(j-i-1));
}

/**************function*********************/
/**
 * @file		
 * @date		2017-4 
 */
int16 partition(int16 arr[], int16 low, int16 high)
{
    int16 key;
    key = arr[low];
    while(low<high){
        while(low <high && arr[high]>= key )
            high--;
        if(low<high)
            arr[low++] = arr[high];
        while( low<high && arr[low]<=key )
            low++;
        if(low<high)
            arr[high--] = arr[low];
    }
    arr[low] = key;
    return low;
}

/**************function*********************/
/**
 * @file		快速排序
 * @date		2017-4 
 */
void quick_sort(int16 arr[], int16 start, int16 end)
{
    int16 pos;
    if (start<end){
        pos = partition(arr, start, end);//
        quick_sort(arr,start,pos-1);	//递归
        quick_sort(arr,pos+1,end);		//递归
    }
    return;
}

/**************function*********************/
/**
 * @file		解算下一行线
 * @date		2017-4 
 */
int16 judge_next_line(int16 sline,int16 eline,int16 start)
{
  while(sline>=eline){
    if(PP(sline,start)==BLACK_P){
      static int16 break_flag;
      break_flag=search_black_line(sline,start);
      if(break_flag==1){
        break;
      }
      if(break_flag==2){
        goto segement;
      }
    }
    if(PP(sline,start)==WHITE_P){
      start=search_white_line(sline,start);
    }
    
    if(start==0||start==159){
      break;
    }
  segement:    
    sline--;
  }
  
  static int16 end_line;
  end_line=sline+1;
  
  return end_line;
}

/**************function*********************/
/**
 * @file		搜黑边线，环岛内侧是黑线
 * @date		2017-4 
 */
int16 search_black_line(int16 line,int16 start)
{
  static int16 i;
  int16 diff[4]={0,159,0,159};
  
  if(car_position==bef_in_circle_left||car_position==bef_in_circle_righ){
    return 1;
  }
  
  if(car_ten_status==normal_ten&&car_position==normal){
    if(start<=100&&start>=60){
      for(i=start;i>=19;i--){
        if(PP(line,i)==BLACK_P&&PP(line,i-1)==WHITE_P&&PP(line,i-2)==WHITE_P){
          diff[0]=i;
          break;
        }
      }
      for(i=start;i<=140;i++){
        if(PP(line,i)==BLACK_P&&PP(line,i+1)==WHITE_P&&PP(line,i+2)==WHITE_P){
          diff[1]=i;
          break;
        }      
      }
      for(i=start;i>=19;i--){
        if(PP(line-1,i)==BLACK_P&&PP(line-1,i-1)==WHITE_P&&PP(line-1,i-2)==WHITE_P){
          diff[2]=i;
          break;
        }
      }    
      for(i=start;i<=140;i++){
        if(PP(line-1,i)==BLACK_P&&PP(line-1,i+1)==WHITE_P&&PP(line-1,i+2)==WHITE_P){
          diff[3]=i;
          break;
        }      
      }    
      
      if(diff[0]!=0&&diff[1]!=159&&diff[2]!=0&&diff[3]!=159){
        if(diff[0]<80&&diff[2]<80&&diff[1]>80&&diff[3]>80){
          if((diff[0]>=diff[2]+1)&&(diff[1]<=diff[3]-1)){
            if(turn_flag==LEFT){
              car_position=bef_in_circle_left;
            }
            if(turn_flag==RIGH){
              car_position=bef_in_circle_righ;
            }
          }
        }
      }
    }
    return 1;
  }
  
  if(car_position==aft_in_circle_left){
    static int16 k;
    for(k=start;k>=1;k--){
      if(PP(line,k)==BLACK_P&&PP(line,k-1)==WHITE_P){
        break;
      }
    }
    SL(line)=k/2;
    LL(line)=0;
    RL(line)=k;
    return 2;
  }
  
  return 1;
}

/**************function*********************/
/**
 * @file		搜白边线
 * @date		2017-4 
 */
int16 search_white_line(int16 line,int16 start)
{
  find_line(line,start);
  return judge_column(line);
}

/**************function*********************/
/**
 * @file		
 * @date		2017-4 
 */
int16 judge_column(int16 line)
{
  if(LL(line)!=0&&RL(line)!=159){
    if(((LL(line)+RL(line))/2-SL(line+1))>=0){
      SL(line)=SL(line+1)+(((LL(line)+RL(line))/2-SL(line+1))>=2?2:((LL(line)+RL(line))/2-SL(line+1)));
    }
    if(((LL(line)+RL(line))/2-SL(line+1))<=0){
      SL(line)=SL(line+1)+(((LL(line)+RL(line))/2-SL(line+1))<=(-2)?(-2):((LL(line)+RL(line))/2-SL(line+1)));
    }
    if(car_position==normal){
      lost_to_have_flag=0;
    }
  }
  
  if(LL(line)!=0&&RL(line)==159){
    if((LL(line)-LL(line+1))>=0){
      SL(line)=SL(line+1)+((LL(line)-LL(line+1))>=2?2:(LL(line)-LL(line+1)));
    } 
    else{
      if(car_position==normal){
        SL(line)=SL(line+1)+1;
      }
      else{
        SL(line)=SL(line+1)+((LL(line)-LL(line+1))<=(-2)?(-2):(LL(line)-LL(line+1)));
      }
    }
    if(lost_to_have_flag==1&&car_position==normal){
      SL(line)=SL(line+1);
    }    
  }  

  if(LL(line)==0&&RL(line)!=159){
    if((RL(line)-RL(line+1))<=0){
      SL(line)=SL(line+1)+((RL(line)-RL(line+1))<=(-2)?(-2):(RL(line)-RL(line+1)));
    }
    else{
      if(car_position==normal){
        SL(line)=SL(line+1)-1;
      }
      else{
        SL(line)=SL(line+1)+((RL(line)-RL(line+1))>=2?2:(RL(line)-RL(line+1)));
      }      
    }
    if(lost_to_have_flag==1&&car_position==normal){
      SL(line)=SL(line+1);
    }      
  }

  if(LL(line)==0&&RL(line)==159){
    SL(line)=SL(line+1);
    if(car_position==normal){
      lost_to_have_flag=1;      
    }
    if(near_lost_count_left!=0&&near_lost_count_righ!=0){
      SL(line)=80;
    }
    lost_count++;
    if(lost_count>=15){
      SL(line)=80;
    }      
  }

  SL(line)=SL(line)>=159?159:SL(line);
  SL(line)=SL(line)<=  0?  0:SL(line);
  
  PP(line,SL(line))=0;
  return SL(line);  
}

/**************function*********************/
/**
 * @file		判断车的位置和状态
 * @date		2017-4 
 */
void judge_position(void)
{
  if(turn_flag==LEFT){
    if(car_position==bef_in_circle_left&&near_lost_count_left==0&&near_lost_count_righ!=0){
      car_position=mid_in_circle_left;
    }
    if(car_position==mid_in_circle_left&&near_lost_count_left==0&&near_lost_count_righ==0){
      car_position=aft_in_circle_left;
    }  
    if(car_position==aft_in_circle_left&&near_lost_count_left==0&&near_lost_count_righ!=0){
      car_position=mid_out_circle_left;
    }    
    if(car_position==mid_out_circle_left&&near_lost_count_left==0&&near_lost_count_righ==0){
      car_position=aft_out_circle_left;
    }
    if(car_position==aft_out_circle_left&&near_lost_count_left==0&&near_lost_count_righ==0){
      car_position=normal;
    }    
  }
  
  if(car_position==normal&&near_lost_count_left!=0&&near_lost_count_righ!=0&&car_ten_status==normal_ten){
    car_ten_status=bef_ten;
  }
  if(car_position==normal&&near_lost_count_left==0&&near_lost_count_righ==0&&car_ten_status==bef_ten){
    car_ten_status=mid_ten;
  }
  if(car_position==normal&&near_lost_count_left!=0&&near_lost_count_righ!=0&&car_ten_status==mid_ten){
    car_ten_status=aft_ten;
  } 
  if(car_position==normal&&near_lost_count_left==0&&near_lost_count_righ==0&&car_ten_status==aft_ten){
    car_ten_status=normal_ten;
  }   
}

