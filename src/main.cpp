//#include "main.hpp"
#include "stm32h753xx.h"
#include "core_cm7.h"
#include "cmsis_compiler.h"

#include <stmcpp/register.hpp>
#include <stmcpp/clock.hpp>
#include <stmcpp/gpio.hpp>
#include <stmcpp/units.hpp>

using namespace stmcpp::units;

stmcpp::gpio::pin<stmcpp::gpio::port::portb, 14> red(stmcpp::gpio::mode::output);


extern "C" void SystemInit(void){
	// Enable the FPU if needed
	#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
		stmcpp::reg::set(std::ref(SCB->CPACR), (3UL << 20U) | (3UL << 22U));
    #endif

	// Enable the necessary peripheral clocks
	stmcpp::clock::enablePeripherals(
		stmcpp::clock::peripheral::gpioe,
		stmcpp::clock::peripheral::gpiob
	);
}



extern "C" int main(void){
	// Enable the systick to run at 1ms
	stmcpp::clock::systick::enable(64_MHz, 1_ms);
	
	while(1){
		stmcpp::clock::systick::waitBlocking(500_ms);
		red.toggle();
	}
	
}

// Increment the systick timer
extern "C" void SysTick_Handler(){
    stmcpp::clock::systick::increment();
}

extern "C" void NMI_Handler(void) {
	// If the interrupt was triggered by HSE clock 
	if (stmcpp::reg::read(std::ref(RCC->CIFR), RCC_CIFR_HSECSSF)) {

		// Deal with it later, just loop endlessly for now
		while (true) {;}
	}
}

void stmcpp::error::globalFaultHandler(std::uint32_t hash, std::uint32_t code) {
	// Deal with it later, just loop endlessly for now
	while (true) {;}
}


extern "C" void HardFault_Handler(void){
	//Ooops, hard fault! Disable interrupts
	__disable_irq();

	std::uint32_t hfsr = stmcpp::reg::read(std::ref(SCB->HFSR));

	if (hfsr & SCB_HFSR_FORCED_Msk) {
		// The hardfault has been forced because of other fault (Mem, Usage or Bus), skip to the other breakpoints
		__ASM volatile("bkpt");
	} else if  (hfsr & SCB_HFSR_VECTTBL_Msk) {
		// The hardfault has been caused 
		__ASM volatile("bkpt");
	}

	std::uint32_t cfsr = stmcpp::reg::read(std::ref(SCB->CFSR));

	if (cfsr & SCB_CFSR_DIVBYZERO_Msk) {
		// You have tried to divide by zero
		__ASM volatile("bkpt");
	} else if (cfsr & SCB_CFSR_UNALIGNED_Msk) {
		// Unaligned access to memory
		__ASM volatile("bkpt");
	} else if (cfsr & SCB_CFSR_NOCP_Msk) {
		// You are trying to use a disabled coprocessor, is the FPU enabled?
		__ASM volatile("bkpt");
	} else if (cfsr & SCB_CFSR_UNDEFINSTR_Pos) {
		// Woah, undefined instruction, isn't the stack corrupted?
		__ASM volatile("bkpt");
	} else {
		/*
		Hmmm, something else went wrong... What now?
		Check the other CFSR bits
			INVPC
			INVSTATE - can occur 99% only if writing hand assembly
			BFARVALID - indicates that BFAR holds the address causing the fault
			LSPERR & STKERR - might occur in case of stack overflow
			UNSTKERR - fault while returning from except (corrupted stack)
			IMPRECISERR - was the MCU able to determine the exact fault location?
			PRECISERR - instruction executed befor exception caused the fault
		*/

		__ASM volatile("bkpt");
	}
}

