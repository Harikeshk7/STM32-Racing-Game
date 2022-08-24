
/**
  ******************************************************************************
  * @file	main.c
  * @author  Ac6
  * @version V1.0
  * @date	01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/

#include "stm32f0xx.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>



uint8_t matrix[64][32];
uint8_t car_row = 63;
uint8_t score_val = 0;
bool movement_complete = false;
bool start_game = false;
int obstactle_gen = 0;
int current_pos[2];
bool endgame = false;
bool first_clear = true;
uint8_t speed_offset = 0;
int mph_val = 50;
bool car_movement = true;
bool sound_effect = false;
int effect_offset = 0;
int count_start = 0;
int counting = 3;

void init_tim17(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_TIM17EN;
	TIM17->PSC = 1-1;
	TIM17->ARR = 60000;

	TIM17->DIER |= TIM_DIER_UIE;
	TIM17->CR1 |= TIM_CR1_CEN;
	NVIC->ISER[0] = 1<<TIM17_IRQn;
	NVIC_SetPriority(TIM17_IRQn, 2);

}

void TIM17_IRQHandler(void){
	int count = 0;
	TIM17->SR &=~ 1;
	for(int col = 0; col < 32; col++){
    	for(int row = 0; row < 64; row++){
        	//GPIOC->ODR = (matrix[row][col] & 0x3F);
        	GPIOC->BSRR |= (matrix[row][col] & 0x3F); // update pixel
        	GPIOC->BSRR |= 1 << 6; //clock on
        	GPIOC->BRR |= 1 << 6; //clock off
        	GPIOC->BRR |= 0x3f;
    	}
    	GPIOB->BSRR |= 1<<2; //turn on OE
    	GPIOC->BRR |= (0x1f)<<8; // reset A-E
    	GPIOC->BSRR |= (col) << 8; // update A-E
    	GPIOC->BSRR |= 1<< 7; // turn on strb
    	GPIOC->BRR |= 1<< 7; // turn off strb
    	GPIOB->BRR |= 1<< 2; // Turn off off OE
	}

	if(start_game){
    	//init_tim14();
    	srandom(TIM17->CNT);
    	if (first_clear){
       	clear_matrix(5,0,0);
       	first_clear = false;
    	}
    	score();
    	mph();
    	generate_lanes();
    	if(score_val < 101){
        	movement(car_movement);
    	}
    	else{
        	end_game();
    	}

    	if (movement_complete == true){
            	generate_cars();
            	movement_complete = false;
        	}

 	}
	else if(endgame){
    	disable_timer();
    	end_game();
	}
}

void draw_matrix(int row, int col, uint8_t color_byte){
	matrix[row][col] = color_byte;
}


void led_gpio_set(void){
 	/*outputs:
	CLK -> PC6
	STRB -> PC7
	OE-> PB2 (Pull Down)
 	inputs:
      	R1-> PC0
 	R2-> PC3
 	G1-> PC1
 	G2-> PC4
 	B1-> PC2
 	B2-> PC5
 	A-> PC8
 	B-> PC9
 	C-> PC10
 	D-> PC11
 	E-> PC12*/

	//  Enabling clock for Port C
	RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
	// Clearing
	GPIOC->MODER &= ~(GPIO_MODER_MODER6 | GPIO_MODER_MODER7 | GPIO_MODER_MODER14);
	GPIOC->MODER |= (GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0 | GPIO_MODER_MODER14_0);
	GPIOC->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1 | GPIO_MODER_MODER2
        	| GPIO_MODER_MODER3 | GPIO_MODER_MODER4 | GPIO_MODER_MODER5
        	| GPIO_MODER_MODER8 | GPIO_MODER_MODER9 | GPIO_MODER_MODER10
        	|GPIO_MODER_MODER11 | GPIO_MODER_MODER12);
	GPIOC->MODER |= (GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0
            	| GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0
            	| GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0 | GPIO_MODER_MODER10_0
            	|GPIO_MODER_MODER11_0 | GPIO_MODER_MODER12_0);
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	GPIOB->MODER |= GPIO_MODER_MODER2_0;

 }

void draw_car(int row, int col, uint8_t car_color1, uint8_t car_color2, uint8_t car_color3){
	int temp_col = col;
	int temp_row = row;
	for(col = temp_col; col < temp_col+5; col++){
    	for(row = temp_row; row < temp_row+5; row++){
        	if (((row == temp_row) && (col == temp_col)) || ((row == temp_row+4) && (col == temp_col)) || ((row == temp_row) && (col == temp_col + 4)) || ((row == temp_row + 4) && (col == temp_col + 4)) || ((row == temp_row + 2) && (col == temp_col)) || ((row == temp_row + 2) && (col == temp_col + 4))){
            	draw_matrix(row,col,0x0); // BLANK SPACES
        	}
        	else if (((row == temp_row+1) && (col == temp_col)) || ((row == temp_row+3) && (col == temp_col)) || ((row == temp_row+1) && (col == temp_col+4)) || ((row == temp_row+3) && (col == temp_col+4)) || (row == temp_row && (col ==temp_col+2)) || ((row == temp_row+4) && (col == temp_col+2))){
            	draw_matrix(row,col,car_color3); // EXTRA STUFF BLUE
        	}
        	else if (((row == temp_row) && (col == temp_col+3)) || ((row == temp_row) && (col == temp_col+1))){
            	draw_matrix(row,col,car_color2); // BACKLIGHT OF CAR YELLOW
        	}

        	else{
            	draw_matrix(row,col,car_color1); // BODY OF CAR
        	}
    	}
	}

}

void movement(bool car_movement){
	if (car_movement){
    	uint8_t temp_matrix[64][32];

    	for(int i = 0; i < 32; i++){
        	for(int j = 0; j <64; j++){
            	temp_matrix[j][i] = matrix[j][i];
        	}
    	}

	//draw_matrix(9,7,0x3);
    	for (int i = 0; i < 32; i++){
        	for(int j = 57; j > 0; j--){
            	if (j == 63){
                	matrix[j][i] = 0x0;
            	}
            	if ((j <= current_pos[0]+5) && (i >= current_pos[1] && i <= current_pos[1] + 4)){
                	continue;
            	}

            	else{
                	if (((matrix[4][6] == 0x1) && (matrix[5][6] == 0x3)) || ((matrix[4][16] == 0x1) && (matrix[5][16] == 0x3)) || ((matrix[4][26] == 0x1) && (matrix[5][26] == 0x3)) || ((matrix[4][4] == 0x8) && (matrix[5][4] == 0x18)) || ((matrix[4][14] == 0x8) && (matrix[5][14] == 0x18)) || ((matrix[4][24] == 0x8) && (matrix[5][24] == 0x18))){
                    	start_game = false;
                    	endgame = true;
                	}
                	else if ((matrix[4][6] == 0x1 && (matrix[5][6] == 0x7 || matrix[5][6] == 0x6)) || (matrix[4][16] == 0x1 && (matrix[5][16] == 0x7 || matrix[5][16] == 0x6)) || (matrix[4][26] == 0x1 && (matrix[5][26] == 0x7 || matrix[5][26] == 0x6)) || (matrix[4][4] == 0x8 && (matrix[5][4] == 0x38 || matrix[5][4] == 0x30)) || (matrix[4][14] == 0x8 && (matrix[5][14] == 0x38 || matrix[5][14] == 0x30)) || (matrix[4][24] == 0x8 && (matrix[5][14] == 0x38 || matrix[5][14] == 0x30))){
                    	mph_val = 20;
                    	speed_offset = 0;
                	}

                    	matrix[j-1][i] = temp_matrix[j][i];
            	}

        	}
    	}
    	car_row--;
    	//int flag_sc = 0;

    	if (car_row == 15){
        	generate_obstacle();
    	}
    	if (car_row == 0){ // change to car_row = 0

        	//flag_sc = 1;
        	movement_complete = true;
        	numbers(score_val++);
        	disp_mph(mph_val);
        	car_row = 64 + speed_offset;

    	}
	}
//
}
void start_screen(){
	int col;
	int row;
	// Start word

	for (col = 3; col < 12; col++){
    	draw_matrix(59, col, 0x3);
	}
	for (row = 59; row > 50; row--){
    	draw_matrix(row, 2, 0x3);
	}
	for (col = 3; col < 12; col++){
    	draw_matrix(51, col, 0x3);
	}
	for (row = 50; row > 43; row--){
    	draw_matrix(row, 11, 0x3);
	}
	for (col = 2; col < 12; col++){
    	draw_matrix(43, col, 0x3);
	} // END OF S

	// START OF T
	for (col = 13; col < 24; col++){
    	draw_matrix(59, col, 0x3);
	}
	for (row = 59; row > 42; row--){
    	draw_matrix(row, 18, 0x3);
	}

	// First half of A

	for (row = 59; row > 42; row--){
    	draw_matrix(row, 26, 0x3);
	}

	for (col = 26; col < 31; col++){
    	draw_matrix(59, col, 0x3);
	}
	for (col = 25; col < 31; col++){
    	draw_matrix(52, col, 0x3);
	}
	for (col = 0; col < 6; col++){
    	if (matrix[59][col] == 0x0){
        	draw_matrix(59, col, 0x18);
    	}
    	else{
        	draw_matrix(59, col, 0x1b);
    	}
	}
	draw_matrix(59, 31, 0x3);
	draw_matrix(52, 31, 0x3);
	draw_matrix(52, 6, 0x18);
	for (col = 0; col < 5; col++){
    	if (matrix[52][col] == 0x0){
        	draw_matrix(52, col, 0x18);
    	}
    	else{
        	draw_matrix(52, col, 0x1b);
    	}
	}
	for (row = 59; row > 42; row--){
    	if (matrix[row][5] == 0x0){
        	draw_matrix(row,5,0x18);
    	}
    	else{
        	draw_matrix(row, 5, 0x1b);
    	}
	}

//	Start of R

	for (row = 59; row > 42; row--){
    	if (matrix[row][8] == 0x0){
                	draw_matrix(row, 8, 0x18);
            	}
    	else{
        	draw_matrix(row, 8, 0x1b);
    	}
	}
	for (col = 9; col < 19; col++){
    	if (matrix[59][col] == 0x0){
                	draw_matrix(59, col, 0x18);
            	}
    	else{
        	draw_matrix(59, col, 0x1b);
    	}
	}
	for (row = 58; row > 52; row--){
    	if (matrix[row][18] == 0x0){
                	draw_matrix(row, 18, 0x18);
            	}
    	else{
        	draw_matrix(row, 18, 0x1b);
    	}
	}
	for (col = 9; col < 18; col++){
    	if (matrix[53][col] == 0x0){
                	draw_matrix(53, col, 0x18);
            	}
    	else{
        	draw_matrix(53, col, 0x1b);
    	}
	}
	for (row = 52; row > 42; row--){
    	if (matrix[row][17] == 0x0){
                	draw_matrix(row, 17, 0x18);
            	}
    	else{
        	draw_matrix(row, 17, 0x1b);
    	}
	}
	//start of T

	for (col =  21; col < 30; col++){
    	if (matrix[59][col] == 0x0){
                	draw_matrix(59, col, 0x18);
            	}
    	else{
        	draw_matrix(59, col, 0x1b);
    	}
	}
	for (row = 58; row > 42; row--){
    	if (matrix[row][25] == 0x0){
                	draw_matrix(row, 25, 0x18);
            	}
    	else{
        	draw_matrix(row, 25, 0x1b);
    	}
	}
}
void generate_cars(){
	bool first_draw = false;
	bool second_draw = false;
	bool third_draw = false;
	bool fourth_draw = false;
	bool fifth_draw = false;
	bool sixth_draw = false;

	srand(rand());
	int choose_lane = 0;
	uint16_t num_lanes = 0x00;
	while ((num_lanes < 0x1) || (num_lanes > 0x3f)){
    	num_lanes = rand()%(0x3f) + 1;
	}
	while ((choose_lane < 1) || (choose_lane > 6)){
    	choose_lane = rand()%6 + 1;
	}

	if ((num_lanes & 0x1) == 1){ // LANE 1
    	draw_car(52, 5, 0x2, 0x3, 0x4);
    	first_draw = true;

	}
	if (((num_lanes>>1) & 0x1) == 1){ // LANE 2
    	draw_car(52, 15, 0x2, 0x3, 0x4);
    	second_draw = true;
	}
	if (((num_lanes>>2) & 0x1) == 1){ // LANE 3
    	draw_car(52, 25, 0x2, 0x3, 0x4);
    	third_draw = true;
	}
	if ((((num_lanes>>3) & 0x1) == 1) && (!first_draw)){ // LANE 4
    	draw_car(52, 3, 0x10, 0x18, 0x20);
    	fourth_draw = true;
	}
	if ((((num_lanes>>4) & 0x1) == 1) && (!second_draw)){ // LANE 5
    	draw_car(52, 13, 0x10, 0x18, 0x20);
    	fifth_draw = true;
	}
	if ((((num_lanes>>5) & 0x1) == 1) && (!third_draw)){ // LANE 6
    	draw_car(52, 23, 0x10, 0x18, 0x20);
    	sixth_draw = true;
	}
}


void generate_lanes(){
	for (int i = 57; i >= 0; i--){
    	draw_matrix(i, 2, 0x7);
	}
	for(int i = 57; i >= 0; i--){
    	draw_matrix(i, 12, 0x7);
	}
	for(int i = 57; i >= 0; i--){
    	draw_matrix(i, 22, 0x7);
	}
	for (int i = 57; i >= 0; i--){
    	draw_matrix(i,0,0x38);
	}
	for(int i = 57; i >= 0; i--){
    	draw_matrix(i, 10, 0x38);
	}
	for(int i = 57; i >= 0; i--){
    	draw_matrix(i, 30, 0x38);
	}

	for(int i = 57; i >= 0; i--){
    	draw_matrix(i, 20, 0x38);
	}
	for (int i = 2; i < 31; i++){
    	draw_matrix(58,i, 0x3f);
	}
//	for (int i = 2; i < 31; i++){
//    	draw_matrix(58,i, 0x7);
//	}
	draw_matrix(58, 0, 0x38);
	draw_matrix(58, 1, 0x38);
	draw_matrix(58, 31, 0x7);

	//Clearing blue lines
}
void end_game(void){
	endgame = true;
	start_game = false;
	disable_music();
	final_screen();
}


void change_speed(int speed){

	if(speed == 1){
    	if (mph_val <= 80){
        	speed_offset -= 5;
        	mph_val += 10;
    	}
	}
	else if (speed == 0){
    	mph_val -= 10;
	}

}
void change_lane(int val){
	if ((val == 1) && ((matrix[current_pos[0]+2][current_pos[1]+2]) & 0x7) > 0){
    	sound_effect = true;
    	effect_offset = 0;
    	draw_car(current_pos[0], current_pos[1], 0x0, 0x0, 0x0);
    	if (current_pos[1] == 5 ){
        	current_pos[1] = 15;
        	draw_car(current_pos[0], current_pos[1], 0x1, 0x3, 0x4);
    	}
    	else if (current_pos[1] == 15){
        	current_pos[1] = 25;
        	draw_car(current_pos[0], current_pos[1], 0x1, 0x3, 0x4);
    	}
    	else if (current_pos[1] == 25){
        	current_pos[1] = 3;
        	draw_car(current_pos[0], current_pos[1], 0x8, 0x18, 0x20);
    	}

	}
	else if ((val == 1) && (((matrix[current_pos[0]+2][current_pos[1]+2])>>3) & 0x7) > 0){
    	draw_car(current_pos[0], current_pos[1], 0x0, 0x0, 0x0);
    	sound_effect = true;
    	effect_offset = 0;
    	if (current_pos[1] == 3){
        	current_pos[1] = 13;
        	draw_car(current_pos[0], current_pos[1], 0x8, 0x18, 0x20);
    	}
    	else if (current_pos[1] == 13){
        	current_pos[1] = 23;
        	draw_car(current_pos[0], current_pos[1], 0x8, 0x18, 0x20);
    	}
    	else if (current_pos[1] == 23){
        	current_pos[1] = 23;
        	draw_car(current_pos[0], current_pos[1], 0x8, 0x18, 0x20);
    	}
	}
	if ((val == 0) && ((matrix[current_pos[0]+2][current_pos[1]+2]) & 0x7) > 0){
    	draw_car(current_pos[0], current_pos[1], 0x0, 0x0, 0x0);
    	sound_effect = true;
    	effect_offset = 0;
    	if (current_pos[1] == 15 ){
        	draw_car(current_pos[0], current_pos[1], 0x0, 0x0, 0x0);
        	current_pos[1] = 5;
        	draw_car(current_pos[0], current_pos[1], 0x1, 0x3, 0x4);
    	}
    	else if (current_pos[1] == 25){
        	current_pos[1] = 15;
        	draw_car(current_pos[0], current_pos[1], 0x1, 0x3, 0x4);
    	}
    	else if (current_pos[1] == 5){
        	current_pos[1] = 5;
        	draw_car(current_pos[0], current_pos[1], 0x1, 0x3, 0x4);
    	}
	}

   else if (val == 0 && (((matrix[current_pos[0]+2][current_pos[1]+2])>>3) & 0x7) > 0){
    	draw_car(current_pos[0], current_pos[1], 0x0, 0x0, 0x0);
    	sound_effect = true;
    	effect_offset = 0;
    	if (current_pos[1] == 3){
        	current_pos[1] = 25;
        	draw_car(current_pos[0], current_pos[1], 0x1, 0x3, 0x4);
    	}
    	else if (current_pos[1] == 23){
        	current_pos[1] = 13;
        	draw_car(current_pos[0], current_pos[1], 0x8, 0x18, 0x20);
    	}
    	else if (current_pos[1] == 13){
        	current_pos[1] = 3;
        	draw_car(current_pos[0], current_pos[1], 0x8, 0x18, 0x20);
    	}
	}

}

void final_screen(){
	for(int x = 63; x >= 0; x--){
    	for (int y= 0; y < 33; y ++){
        	draw_matrix(x,y,0x0);
    	}
	}
	//G
	for (int x = 58; x >= 44; x--){
    	draw_matrix(x, 6, 0x3);
	}
	for (int y = 7; y < 16; y++){
    	draw_matrix(58, y, 0x3);
	}
	for (int y = 10; y < 16; y++){
    	draw_matrix(51, y, 0x3);
	}
	for (int y = 7; y < 16; y++){
    	draw_matrix(44, y, 0x3);
	}
	for (int x = 50; x > 43; x--){
    	draw_matrix(x, 15, 0x3);
	}


	//A
	for (int x = 58; x >= 44; x--){
    	draw_matrix(x, 19, 0x3);
	}
	for (int y = 19; y < 29; y++){
    	draw_matrix(58, y, 0x3);
	}
	for (int y = 19; y < 29; y++){
    	draw_matrix(51, y, 0x3);
	}

	for (int x = 51; x > 43; x--){
    	draw_matrix(x, 29, 0x3);
	}
	for (int x = 58; x > 51; x--){
    	draw_matrix(x, 29, 0x3);
	}

	//M
	for (int x = 57; x >= 44; x--){
    	if (matrix[x][1] == 0x0){
        	draw_matrix(x, 1, 0x18);
    	}
    	else{
        	draw_matrix(x,1, 0x1b);
    	}
	}
	int y_down = 1;
	for (int x = 58; x >= 53; x--){
    	if (matrix[x][y_down] == 0x0){
        	draw_matrix(x, y_down++, 0x18);
    	}
    	else{
        	draw_matrix(x,y_down++, 0x1b);
    	}

	}
	int y_up = 6;
	for (int x = 53; x <= 58; x++){
    	if (matrix[x][y_up] == 0x0){
        	draw_matrix(x,y_up++,0x18);
    	}
    	else{
        	draw_matrix(x,y_up++, 0x1b);
    	}

	}
	for (int x = 51; x > 43; x--){
    	if (matrix[x][11] == 0x0){
        	draw_matrix(x, 11, 0x18);
    	}
    	else{
        	draw_matrix(x,11, 0x1b);
    	}

	}
	for (int x = 58; x > 51; x--){
    	if (matrix[x][11] == 0x0){
        	draw_matrix(x, 11, 0x18);
    	}
    	else{
        	draw_matrix(x,11, 0x1b);
    	}

	}
	//E
	for (int x = 58; x >= 44; x--){
    	if (matrix[x][15] == 0x0){
        	draw_matrix(x, 15, 0x18);
    	}
    	else{
        	draw_matrix(x,15, 0x1b);
    	}

	}
	for (int y = 16; y < 25; y++){
    	if (matrix[58][y] == 0x0){
        	draw_matrix(58, y, 0x18);
    	}
    	else{
        	draw_matrix(58,y, 0x1b);
    	}

	}
	for (int y = 16; y < 25; y++){
    	if (matrix[51][y] == 0x0){
        	draw_matrix(51, y, 0x18);
    	}
    	else{
        	draw_matrix(51,y, 0x1b);
    	}

	}
	for (int y = 16; y < 25; y++){
    	if (matrix[44][y] == 0x0){
        	draw_matrix(44, y, 0x18);
    	}
    	else{
        	draw_matrix(44,y, 0x1b);
    	}

	}

	//O
	for (int x = 40; x >= 26; x--){
    	draw_matrix(x, 6, 0x3);
	}
	for (int y = 7; y < 16; y++){
    	draw_matrix(40, y, 0x3);
	}

	for (int y = 7; y < 16; y++){
    	draw_matrix(26, y, 0x3);
	}
	for (int x = 32; x > 25; x--){
    	draw_matrix(x, 15, 0x3);
	}
	for (int x = 40; x > 32; x--){
    	draw_matrix(x, 15, 0x3);
	}
	//V
	for (int x = 40; x >= 31; x--){
    	draw_matrix(x, 19, 0x3);
	}
	int v_down = 19;
	for (int x = 30; x >= 26; x--){
    	draw_matrix(x,v_down++,0x3);
	}
	draw_matrix(26, 24, 0x3);
	int v_up = 25;
	for (int x = 26; x <= 30; x++){
    	draw_matrix(x, v_up++, 0x3);
	}
	for (int x = 31; x <= 40; x++){
    	draw_matrix(x, 29, 0x3);
	}
	//E
	for (int x = 40; x >= 26; x--){
    	if (matrix[x][1] == 0x0){
        	draw_matrix(x, 1, 0x18);
    	}
    	else{
        	draw_matrix(x,1, 0x1b);
    	}

	}
	for (int y = 2; y < 12; y++){
    	if (matrix[40][y] == 0x0){
        	draw_matrix(40, y, 0x18);
    	}
    	else{
        	draw_matrix(40,y, 0x1b);
    	}

	}
	for (int y = 2; y < 12; y++){
    	if (matrix[33][y] == 0x0){
        	draw_matrix(33, y, 0x18);
    	}
    	else{
        	draw_matrix(33,y, 0x1b);
    	}

	}
	for (int y = 2; y < 12; y++){
    	if (matrix[26][y] == 0x0){
        	draw_matrix(26, y, 0x18);
    	}
    	else{
        	draw_matrix(26,y, 0x1b);
    	}

	}
	// R
	for (int x = 40; x >= 26; x--){
    	if (matrix[x][15] == 0x0){
                	draw_matrix(x, 15, 0x18);
            	}
    	else{
        	draw_matrix(x, 15, 0x1b);
    	}
	}
	for (int y = 16; y < 24; y++){
    	if (matrix[40][y] == 0x0){
                	draw_matrix(40, y, 0x18);
            	}
    	else{
        	draw_matrix(40, y, 0x1b);
    	}
	}
	for (int x = 40; x >=34; x--){
    	if (matrix[x][24] == 0x0){
                	draw_matrix(x, 24, 0x18);
            	}
    	else{
        	draw_matrix(x, 25, 0x1b);
    	}
	}
	for (int y = 15; y < 24; y++){
    	if (matrix[34][y] == 0x0){
                	draw_matrix(34, y, 0x18);
            	}
    	else{
        	draw_matrix(34, y, 0x1b);
    	}
	}
	for (int x = 33; x >= 26 ; x--){
    	if (matrix[x][23] == 0x0){
                	draw_matrix(x, 23, 0x18);
            	}
    	else{
        	draw_matrix(x, 23, 0x1b);
    	}
	}

}
void score(void)
{
	int row;
	int col;
//	draw_matrix(58, 31, 0x7);
	for (col = 1; col < 4; col++){
    	draw_matrix(63, col, 0x3);
	}
	for (row = 62; row > 60; row--){
    	draw_matrix(row, 1, 0x3);
	}
	for (col = 2; col < 4; col++){
    	draw_matrix(61, col, 0x3);
	}
	for (row = 60; row > 58; row--){
    	draw_matrix(row, 3, 0x3);
	}
	for (col = 1; col < 3; col++){
    	draw_matrix(59, col, 0x3);
	} // END OF S
	// C
	for (col = 5; col < 8; col++){
    	draw_matrix(63, col, 0x3);
	}
	for (row = 62; row > 58; row--){
    	draw_matrix(row, 5, 0x3);
	}
	for (col = 5; col < 8; col++){
    	draw_matrix(59, col, 0x3);
	}
	// O
	for (col = 9; col < 12; col++){
    	draw_matrix(63, col, 0x3);
	}
	for (row = 62; row > 58; row--){
    	draw_matrix(row, 9, 0x3);
	}
	for (col = 9; col < 12; col++){
    	draw_matrix(59, col, 0x3);
	}
	for (row = 62; row > 59; row--){
    	draw_matrix(row, 11, 0x3);
	}
	// R
	for (col = 13; col < 16; col++){
    	draw_matrix(63, col, 0x3);
	}
	for (row = 62; row > 58; row--){
    	draw_matrix(row, 13, 0x3);
	}
	for (col = 13; col < 16; col++){
    	draw_matrix(61, col, 0x3);
	}
	draw_matrix(62, 15, 0x3);
	draw_matrix(60, 14, 0x3);
	draw_matrix(59, 15, 0x3);

	// E
	for (col = 17; col < 20; col++){
    	draw_matrix(63, col, 0x3);
	}
	for (row = 62; row > 58; row--){
    	draw_matrix(row, 17, 0x3);
	}
	for (col = 17; col < 20; col++){
    	draw_matrix(59, col, 0x3);
	}
	for (col = 18; col < 20; col++){
    	draw_matrix(61, col, 0x3);
	}
	// :
	draw_matrix(62, 21, 0x3);
	draw_matrix(59, 21, 0x3);
	//

}
void numbers(int score_val){
	int first_dig = score_val % 10;
	int second_dig = score_val / 10;

	int row;
	int col;
	// FIRST DIGIT INITAL:
	for (col = 27; col < 30; col++){
     	draw_matrix(63, col, 0x3);
 	}
 	for (row = 62; row > 58; row--){
     	draw_matrix(row, 27, 0x3);
 	}
 	for (col = 27; col < 30; col++){
     	draw_matrix(59, col, 0x3);
 	}
 	for (row = 62; row > 59; row--){
     	draw_matrix(row, 29, 0x3);
 	}
 	draw_matrix(61,28,0x3);

	//Second digit:
	for (col = 23; col < 26; col++){
    	draw_matrix(63, col, 0x3);
	}
	for (row = 62; row > 58; row--){
    	draw_matrix(row, 23, 0x3);
	}
	for (col = 23; col < 26; col++){
    	draw_matrix(59, col, 0x3);
	}
	for (row = 62; row > 59; row--){
    	draw_matrix(row, 25, 0x3);
	}
	draw_matrix(61,24,0x3);


//	switch(first_dig)
	switch(first_dig){
	case -1:
	case 0: draw_matrix(61,28,0x0);
        	break;

	case 1: for(row = 63; row > 58; row--){
            	draw_matrix(row, 27, 0x0);
        	}
        	for(row = 63; row > 58; row--){
            	draw_matrix(row, 28, 0x0);
        	}
        	break;
	case 2: draw_matrix(62, 27, 0x0);
        	draw_matrix(60, 29, 0x0);
        	break;
	case 3: draw_matrix(62, 27, 0x0);
        	draw_matrix(60, 27, 0x0);
        	break;
	case 4: draw_matrix(63, 28, 0x0);
        	draw_matrix(60, 27, 0x0);
        	draw_matrix(59, 27, 0x0);
        	draw_matrix(59, 28, 0x0);
        	break;
	case 5: draw_matrix(62, 29, 0x0);
        	draw_matrix(60, 27, 0x0);
        	break;
	case 6: draw_matrix(63, 28, 0x0);
        	draw_matrix(63, 29, 0x0);
        	draw_matrix(62, 29, 0x0);
        	break;
	case 7: for(row = 63; row > 58; row--){
            	draw_matrix(row, 27, 0x0);
        	}
        	for(row = 63; row > 58; row--){
            	draw_matrix(row, 28, 0x0);
        	}
        	draw_matrix (63, 28, 0x3);
        	draw_matrix (63, 27, 0x3);
        	break;
	case 9: draw_matrix(60, 27, 0x0);
        	draw_matrix(59, 27, 0x0);
        	draw_matrix(59, 28, 0x0);
        	break;
	default: break;
	}

	// Second digit


	switch(second_dig){
	case -1:
	case 0: draw_matrix(61,24,0x0);
        	break;
	case 1: for(row = 63; row > 58; row--){
            	draw_matrix(row, 24, 0x0);
    	}
        	for(row = 63; row > 58; row--){
            	draw_matrix(row, 23, 0x0);
    	}
        	break;

	case 2: draw_matrix(62, 23, 0x0);
        	draw_matrix(60, 25, 0x0);
        	break;
	case 3: draw_matrix(62, 23, 0x0);
        	draw_matrix(60, 23, 0x0);
        	break;
	case 4: draw_matrix(63, 24, 0x0);
        	draw_matrix(60, 23, 0x0);
        	draw_matrix(59, 23, 0x0);
        	draw_matrix(59, 24, 0x0);
        	break;
	case 5: draw_matrix(62, 25, 0x0);
        	draw_matrix(60, 23, 0x0);
        	break;
	case 6: draw_matrix(63, 24, 0x0);
        	draw_matrix(63, 25, 0x0);
        	draw_matrix(62, 25, 0x0);
        	break;
	case 7: for(row = 63; row > 58; row--){
            	draw_matrix(row, 23, 0x0);
        	}
        	for(row = 63; row > 58; row--){
            	draw_matrix(row, 24, 0x0);
        	}
        	draw_matrix (63, 24, 0x3);
        	draw_matrix (63, 23, 0x3);
        	break;
	case 9: draw_matrix(60, 23, 0x0);
        	draw_matrix(59, 23, 0x0);
        	draw_matrix(59, 24, 0x0);
        	break;
	default:
           	break;

	}
}
void disp_mph(int speed){
	int first_dig = speed % 10;
	int second_dig = speed / 10;

	int row;
	int col;
	// FIRST DIGIT INITAL:

	uint8_t temp;
//	switch(first_dig)
	switch(first_dig){
	case -1:
	case 0: for (row = 63; row > 58; row--){
        	matrix[row][27] |= 0x8;
        	}
        	for (col = 27; col < 30; col++){
            	matrix[63][col] |= 0x8;
        	}
        	for (row = 63; row > 58; row--){
            	matrix[row][29] |= 0x8;
        	}
        	matrix[59][28] |= 0x8;
        	break;

	case 1: for (row = 64; row > 58; row--){
        	matrix[row][29] |= 0x8;
    	}
        	break;
	case 2:  	for (col = 27; col < 30; col++){
    	matrix[63][col] |= 0x8;
	} 	for (row = 62; row > 60; row--){
    	matrix[row][29] |= 0x8;
	} 	for (col = 27; col < 29; col++){
    	matrix[61][col] |= 0x8;
	} 	for (col = 27; col < 30; col++){
    	matrix[59][col] |= 0x8;
	}for (row = 60; row > 58; row--){
    	matrix[row][27] |= 0x8;
	}
	break;

	case 3: for(row = 63; row > 58; row--){
    	matrix[row][29] |= 0x8;
	}
	for (col = 27; col < 29; col++){
    	matrix[63][col] |= 0x8;
	}
	for (col = 27; col < 29; col++){
    	matrix[61][col] |= 0x8;
	}
	for (col = 27; col < 29; col++){
    	matrix[59][col] |= 0x8;
	}
	break;


	case 4: for(row = 63; row > 58; row--){
    	matrix[row][29] |= 0x8;
	}
	for (col = 27; col < 29; col++){
    	matrix[61][col] |= 0x8;
	}
	for (row = 63; row > 61; row--){
    	matrix[row][27] |= 0x8;
	}
	break;

	case 5: for (row = 62; row > 61; row--){
    	matrix[row][27] |= 0x8;
	}
	for (col = 27; col < 30; col++){
    	matrix[63][col] |= 0x8;
	}
	for (col = 27; col < 30; col++){
    	matrix[61][col] |= 0x8;
	}
	for (col = 27; col < 30; col++){
    	matrix[59][col] |= 0x8;
	}
	matrix[60][29] |= 0x8;
        	break;
	case 6: for (row = 63; row > 58; row--){
    	matrix[row][27] |= 0x8;
	}
	for (col = 28; col < 30; col++){
    	matrix[59][col] |= 0x8;
	}
	for (row = 61; row > 59; row--){
    	matrix[row][29] |= 0x8;
	}
	matrix[61][28] |= 0x8;
        	break;
	case 7: for (col=27; col < 30; col++){
    	matrix[63][col] |= 0x8;
	}
	for (row = 63; row > 58; row--){
    	matrix[row][29] |= 0x8;
	}
        	break;
	case 8:	for (col = 27; col < 30; col++){
    	matrix[63][col] |= 0x8;
	}
	for (row = 62; row > 58; row--){
    	matrix[row][27] |= 0x8;
	}
	for (col = 27; col < 30; col++){
    	matrix[59][col] |= 0x8;
	}
	for (row = 62; row > 59; row--){
    	matrix[row][29] |= 0x8;
	}
	matrix[61][28] |= 0x8;
	break;

	case 9: for(row = 63; row > 58; row--){
    	matrix[row][29] |= 0x8;
	}
	for (col = 27; col < 29; col++){
    	matrix[61][col] |= 0x8;
	}
	for (row = 63; row > 61; row--){
    	matrix[row][27] |= 0x8;
	}
    	matrix[63][28] |= 0x8;
	break;

	default: break;
	}

	// Second digit
	switch(second_dig){
	case -1:
	case 0: for (row = 63; row > 58; row--){
        	matrix[row][23] |= 0x8;
        	}
        	for (col = 23; col < 26; col++){
            	matrix[63][col] |= 0x8;
        	}
        	for (row = 63; row > 58; row--){
            	matrix[row][25] |= 0x8;
        	}
        	matrix[59][24] |= 0x8;
        	break;

	case 1: for (row = 64; row > 58; row--){
        	matrix[row][25] |= 0x8;
    	}
        	break;
	case 2:  	for (col = 23; col < 26; col++){
    	matrix[63][col] |= 0x8;
	} 	for (row = 62; row > 60; row--){
    	matrix[row][25] |= 0x8;
	} 	for (col = 23; col < 26; col++){
    	matrix[61][col] |= 0x8;
	} 	for (col = 23; col < 26; col++){
    	matrix[59][col] |= 0x8;
	}for (row = 60; row > 58; row--){
    	matrix[row][23] |= 0x8;
	}
	break;

	case 3: for(row = 63; row > 58; row--){
    	matrix[row][25] |= 0x8;
	}
	for (col = 23; col < 26; col++){
    	matrix[63][col] |= 0x8;
	}
	for (col = 23; col < 26; col++){
    	matrix[61][col] |= 0x8;
	}
	for (col = 23; col < 26; col++){
    	matrix[59][col] |= 0x8;
	}
	break;


	case 4: for(row = 63; row > 58; row--){
    	matrix[row][25] |= 0x8;
	}
	for (col = 23; col < 26; col++){
    	matrix[61][col] |= 0x8;
	}
	for (row = 63; row > 61; row--){
    	matrix[row][23] |= 0x8;
	}
	break;

	case 5: for (row = 62; row > 61; row--){
    	matrix[row][23] |= 0x8;
	}
	for (col = 23; col < 26; col++){
    	matrix[63][col] |= 0x8;
	}
	for (col = 23; col < 26; col++){
    	matrix[61][col] |= 0x8;
	}
	for (col = 23; col < 26; col++){
    	matrix[59][col] |= 0x8;
	}
	matrix[60][25] |= 0x8;
        	break;
	case 6: for (row = 63; row > 58; row--){
    	matrix[row][23] |= 0x8;
	}
	for (col = 24; col < 26; col++){
    	matrix[59][col] |= 0x8;
	}
	for (row = 61; row > 59; row--){
    	matrix[row][25] |= 0x8;
	}
	matrix[61][24] |= 0x8;
        	break;
	case 7: for (col=23; col < 26; col++){
    	matrix[63][col] |= 0x8;
	}
	for (row = 63; row > 58; row--){
    	matrix[row][25] |= 0x8;
	}
        	break;
	case 8:	for (col = 23; col < 26; col++){
    	matrix[63][col] |= 0x8;
	}
	for (row = 62; row > 58; row--){
    	matrix[row][23] |= 0x8;
	}
	for (col = 23; col < 26; col++){
    	matrix[59][col] |= 0x8;
	}
	for (row = 62; row > 59; row--){
    	matrix[row][25] |= 0x8;
	}
	matrix[61][24] |= 0x8;
	break;

	case 9: for(row = 63; row > 58; row--){
    	matrix[row][25] |= 0x8;
	}
	for (col = 23; col < 26; col++){
    	matrix[61][col] |= 0x8;
	}
	for (row = 63; row > 61; row--){
    	matrix[row][23] |= 0x8;
	}
    	matrix[63][24] |= 0x8;
	break;

	default: break;
	}
}

void mph(void){
	int col;
	int row;
	//start of m
	for (row = 63; row > 58; row--){
    	matrix[row][2] |= 0x8;
    	matrix[row][6] |= 0x8;
	}
	matrix[62][3] |= 0x8;
	matrix[61][4] |= 0x8;
	matrix[62][5] |= 0x8;

	//start of P

	for (row = 63; row > 58; row--){
    	matrix[row][8] |= 0x8;
	}
	for (col = 9; col < 12; col++){
    	matrix[63][col] |= 0x8;
	}
	for (col = 9; col < 12; col++){
    	matrix[61][col] |= 0x8;
	}
	matrix[62][11] |= 0x8;
	// H
	for (row = 63; row > 58; row--){
    	matrix[row][13] |= 0x8;
	}
	for (row = 63; row > 58; row--){
    	matrix[row][16] |= 0x8;
	}

	matrix[61][15]|=0x8;
	matrix[61][14]|=0x8;

	// :
	matrix[62][18] |=0x8;
	matrix[59][18] |=0x8;
}

void generate_obstacle(){
	int obstacle_loc = rand()%6 + 1;
	int obstacle_type = rand()%2 + 1;

	if (obstacle_loc == 1){
    	if (obstacle_type == 1){
        	draw_cone(51, 5, 0x7);
    	}
    	else{
        	draw_pothole(51,5,0x6);
    	}
	}
	else if (obstacle_loc == 2){
    	if (obstacle_type == 1){
        	draw_cone(51, 15, 0x7);
    	}
    	else{
        	draw_pothole(51,15,0x6);
    	}
	}
	else if (obstacle_loc == 3){
    	if (obstacle_type == 1){
        	draw_cone(51, 25, 0x7);
    	}
    	else{
        	draw_pothole(51,25,0x6);
    	}
	}
	else if (obstacle_loc == 4){
    	if (obstacle_type == 1){
        	draw_cone(51, 3, 0x38);
    	}
    	else{
        	draw_pothole(51,3,0x30);
    	}
	}
	else if (obstacle_loc == 5){
    	if (obstacle_type == 1){
        	draw_cone(51, 13, 0x38);
    	}
    	else{
        	draw_pothole(51,13,0x30);
    	}
	}
	else if (obstacle_loc == 6){
    	if (obstacle_type == 1){
        	draw_cone(51, 23, 0x38);
    	}
    	else{
        	draw_pothole(51,23,0x30);
    	}
	}
}



void clear_matrix(int row, int col, int str_col){

    	for (int x = 63; x >= row; x--){
        	for (int y = 0; y <= str_col; y++){
            	draw_matrix(x,y,0x0);
        	}
        	for (int y = col+1; y < 33; y++){
            	draw_matrix(x,y,0x0);
        	}
    	}
    	draw_matrix(59, -1, 0x0);
    	draw_matrix(52, 32, 0x0);


}
void draw_cone(int row, int col, uint8_t obs_color){
	int temp_col = col;
	int temp_row = row;

	for(col = temp_col; col < temp_col + 5; col++){
        	draw_matrix(row, col, obs_color);
	}
	for(col = temp_col+1; col < temp_col + 4; col++){
        	draw_matrix(row+1, col, obs_color);
	}
	for(col = temp_col+2; col < temp_col + 3; col++){
        	draw_matrix(row+2, col, obs_color);
	}
}
void draw_pothole(int row, int col, uint8_t obs_color){
	int temp_col = col;
	int temp_row = row;


	for(col = temp_col; col < temp_col + 3; col++){
        	draw_matrix(row, col, obs_color);
	}

	for(col = temp_col; col < temp_col + 3; col++){
        	draw_matrix(row -3, col, obs_color);
	}

	for(row = temp_row; row >= temp_row - 3; row--)
	{
    	draw_matrix(row, col, obs_color);
	}
	for(row = temp_row; row >= temp_row - 3; row--)
	{
    	draw_matrix(row, col -3, obs_color);
	}
}
void start_counter(int x){
	int col;
	int row;
	switch(x){

	case 1:
    	for(row = 42; row >22; row --){
        	draw_matrix(row, 31, 0x3);
    	}
    	break;
	case 2:
    	for(row = 42; row >31; row --){
        	draw_matrix(row, 7, 0x18);
    	}
    	for(row = 31; row >22; row --){
        	draw_matrix(row, 27, 0x3);
    	}
    	for(col = 6; col >= 0; col --){
        	draw_matrix(42, col, 0x18);
        	draw_matrix(22, col, 0x18);
        	draw_matrix(32, col, 0x18);
    	}
    	for(col = 31; col >= 27; col --){
        	draw_matrix(42, col, 0x3);
        	draw_matrix(22, col, 0x3);
        	draw_matrix(32, col, 0x3);
    	}
    	draw_matrix(22, 7, 0x18);
    	break;

	case 3:
    	for(row = 42; row > 21; row --){
        	draw_matrix(row, 7, 0x18);
    	}
    	for(col = 6; col >= 0; col --){
        	draw_matrix(42, col, 0x18);
        	draw_matrix(22, col, 0x18);
        	draw_matrix(32, col, 0x18);
    	}
    	for(col = 31; col >= 27; col --){
        	draw_matrix(42, col, 0x3);
        	draw_matrix(22, col, 0x3);
        	draw_matrix(32, col, 0x3);
    	}
    	break;
	}


}
void clear_counter(int x){
	int col;
	int row;
	switch(x){

	case 1:
    	for(row = 42; row >22; row --){
        	draw_matrix(row, 31, 0x0);
    	}
    	break;
	case 2:
    	for(row = 42; row >31; row --){
        	draw_matrix(row, 7, 0x0);
    	}
    	for(row = 31; row >22; row --){
        	draw_matrix(row, 27, 0x0);
    	}
    	for(col = 6; col >= 0; col --){
        	draw_matrix(42, col, 0x0);
        	draw_matrix(22, col, 0x0);
        	draw_matrix(32, col, 0x0);
    	}
    	for(col = 31; col >= 27; col --){
        	draw_matrix(42, col, 0x0);
        	draw_matrix(22, col, 0x0);
        	draw_matrix(32, col, 0x0);
    	}
    	draw_matrix(22, 7, 0x0);
    	break;

	case 3:
    	for(row = 42; row > 21; row --){
        	draw_matrix(row, 7, 0x0);
    	}
    	for(col = 6; col >= 0; col --){
        	draw_matrix(42, col, 0x0);
        	draw_matrix(22, col, 0x0);
        	draw_matrix(32, col, 0x0);
    	}
    	for(col = 31; col >= 27; col --){
        	draw_matrix(42, col, 0x0);
        	draw_matrix(22, col, 0x0);
        	draw_matrix(32, col, 0x0);
    	}
    	break;
	}

}
void nano_wait(unsigned int n) {
	asm(	"    	mov r0,%0\n"
        	"repeat: sub r0,#83\n"
        	"    	bgt repeat\n" : : "r"(n) : "r0", "cc");
}
void init_tim14(void){
	RCC->APB1ENR |= RCC_APB1ENR_TIM14EN;
	TIM14->PSC = 10000-1;
	TIM14->ARR = 4800-1;

	TIM14->DIER |= TIM_DIER_UIE;
	TIM14->CR1 |= TIM_CR1_CEN;

	NVIC->ISER[0] = 1<<TIM14_IRQn;
	//NVIC_SetPriority(TIM17_IRQn, 2);
}

void TIM14_IRQHandler(void)
{
	TIM14->SR &= ~0x1;
	clear_matrix(5,0,0);

	if(counting > 0){
    	clear_counter(counting + 1);
    	start_counter(counting);
	}
	else{
//    	start_game = true;
    	TIM14->CR1 &= ~TIM_CR1_CEN;
    	start_game = true;
	}
	counting--;

}


int main(void){

	led_gpio_set(); // Setting the GPIO for I/O to check LED matrix
	current_pos[0] = 0;
	current_pos[1] = 5;
	draw_car(current_pos[0], current_pos[1], 0x1, 0x3, 0x4);
	start_screen();
	//clear_matrix();
//	//init_tim5();
//	start_counter(3);
//	nano_wait(100000000);
//	start_counter(2);
//	nano_wait(100000000);
//	start_counter(1);
//	nano_wait(100000000);

	//draw_pothole(56, 5, 0x30);
	main_i2c();
	midi_main(true);
	init_tim17();

}