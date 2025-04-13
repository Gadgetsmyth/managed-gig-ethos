#include "MdcMdioController.h"

MdcMdioController::MdcMdioController(int mdcPin, int mdioPin)
    : mdcPin(mdcPin), mdioPin(mdioPin) {
}

void MdcMdioController::begin() {
    // Configure MDC and MDIO pins as outputs
    pinMode(mdcPin, OUTPUT);
    pinMode(mdioPin, OUTPUT);
    
    // Initialize pins to low state
    //// digitalWrite(mdcPin, LOW);
    //// digitalWrite(mdioPin, LOW);
    
    // For ATmega328P, ensure we're using the correct port registers
    // PC4 (A4) and PC5 (A5) are on PORTC
    // No additional configuration needed as Arduino's digitalWrite/digitalRead
    // functions handle the port mapping correctly


    // initialize the dual phy!
            // phy setup commands , needs to be done on both halves. 
        // or set broadcast mode register and do it on both at the same time, that's better probably.
    // set rgmii mode set bit 12 register 23 (0x17, default is 0x2000)
        




}


void MdcMdioController::initialize_dual_phy() {

    // configure the dual phy
    // set broadcast mode. ( so we don't have to write both phys)
    writeMdc(0x00, 0x16, 0x3201);
    // do a soft reset
    writeMdc(0x00, 0x00, 0x9040);
    // set RGMII mode
    // writemdc 0x00 0x17 0x3000
    writeMdc(0x00, 0x17, 0x3000);
    // soft reset // set bit 15, default is 0x1040
    // writemdc 0x00 0x00 0x9040
    writeMdc(0x00, 0x00, 0x9040);
    // adjust the clock control
    // configure 20E2 and ( set 31 (0x1F) to 2 ( sets the address space to e2) ) 
    // this means registers 16 through 30 are remapped from the main space.
    writeMdc(0x00, 0x1f, 0x0002);
    // writemdc 0x00 0x1f 0x0002
    // write register 20 (0x14 )in space 2, to adjust the clock delay 
    // bits 2:0 are gtx clock delay and bits 6:4 are the same, 
    // pattern increases by .3ns per bit, with a minimum of .2ns
    // we set it to bit pattern 100 to get 2ns delay,
    // this means we write 0x0055 to the bit field
    // patch uses 0x0011
    // the high nibble is for the rx pair, the low is the tx pair, the switch compentsates the tx already, but the rx needs more delay, 
    // we also need to clear bit 11, the default field is 0x0800 we change it to 0x0031
    // TODO: TUNE THIS NUMBER BETTER< THIS LINKS UP AND TALKS< but requires further testing
    writeMdc(0x00, 0x14, 0x0031);
    // reset the extended field
    writeMdc(0x00, 0x1f, 0x0000);

    writeMdc(0x00, 0x00, 0x9040);
    // turn smi duplication back off
    writeMdc(0x00, 0x16, 0x3200);

}

/*! Generates a rising edge pulse on IP175 MDC */                                    
// void pulse_mdc(void)                                                               
// {                                                                                    
//     digitalWrite(mdcPin, 0);                                                          
//     delay(1);                                                                       
//     digitalWrite(mdcPin, 1);                                                          
//     delay(1);                                                                       
// }              

void MdcMdioController::pulse_mdc(void)                                                               
{                                                                                    
    digitalWrite(mdcPin, 0);                                                          
    delay(1);                                                                       
    digitalWrite(mdcPin, 1);                                                          
    delay(1);                                                                       
}            

void MdcMdioController::startPreamble() {
    // Send 32 bits of preamble (all 1's)
    for (int i = 0; i < 32; i++) {
        digitalWrite(mdioPin, HIGH);
        digitalWrite(mdcPin, LOW);
        delayMicroseconds(1);
        digitalWrite(mdcPin, HIGH);
        delayMicroseconds(1);
    }
}

void MdcMdioController::writeBit(bool bit) {
    digitalWrite(mdioPin, bit);
    digitalWrite(mdcPin, LOW);
    delayMicroseconds(1);
    digitalWrite(mdcPin, HIGH);
    delayMicroseconds(1);
}

bool MdcMdioController::readBit() {

    bool bit = digitalRead(mdioPin);
    digitalWrite(mdcPin, LOW);
    delayMicroseconds(1);
    digitalWrite(mdcPin, HIGH);
    delayMicroseconds(1);
    return bit;
}

void MdcMdioController::turnaround(bool isRead) {
    if (isRead) {
        // For read: MDIO goes high-Z, then input
        pinMode(mdioPin, INPUT);
        digitalWrite(mdcPin, LOW);
        delayMicroseconds(1);
        digitalWrite(mdcPin, HIGH);
        delayMicroseconds(1);
    } else {
        // For write: MDIO stays as output
        digitalWrite(mdcPin, LOW);
        delayMicroseconds(1);
        digitalWrite(mdcPin, HIGH);
        delayMicroseconds(1);
    }
}

uint16_t MdcMdioController::readData() {
    uint16_t data = 0;
    for (int i = 15; i >= 0; i--) {
        data |= (readBit() << i);
    }
    return data;
}

void MdcMdioController::writeData(uint16_t data) {
    for (int i = 15; i >= 0; i--) {
        writeBit((data >> i) & 0x01);
    }
}

uint16_t MdcMdioController::readMdc(uint8_t phyAddr, uint8_t regAddr) {
     uint8_t byte;                                                                    
    uint16_t word, data;                                                             
                                                                                     
    //void (*pulse_mdc)(void);                                                         
                                                                                     
    //pulse_mdc = pulse_mdcip;                                                         
                                                                                     
    /* MDIO pin is output */                                                         
    pinMode(mdioPin, OUTPUT);                                                           
                                                                                     
    digitalWrite(mdioPin, 1);                                                           
    for (byte = 0;byte < 32; byte++)                                                 
       pulse_mdc();                                                                 

    // // preamble
    // pulse_mdc(); 
    // digitalWrite(mdioPin, 1);
    // pulse_mdc();   

    /* Stat code */                                                                  
    digitalWrite(mdioPin, 0);                                                           
    pulse_mdc();                                                                     
    digitalWrite(mdioPin, 1);                                                           
    pulse_mdc();                                                                     
                                                                                     
    /* Read OP Code */                                                               
    digitalWrite(mdioPin, 1);                                                           
    pulse_mdc();                                                                     
    digitalWrite(mdioPin, 0);                                                           
    pulse_mdc();                                                                     
                                                                                     
    /* PHY address - 5 bits */                                                       
    for (byte=0x10;byte!=0;){                                                        
        if (byte & phyAddr)                                                              
            digitalWrite(mdioPin, 1);                                                   
        else                                                                         
            digitalWrite(mdioPin, 0);                                                   
                                                                                     
        pulse_mdc();                                                                 
                                                                                     
        byte=byte>>1;                                                                
    }                                                                                
    /* REG address - 5 bits */                                                       
    for (byte=0x10;byte!=0;){                                                        
        if (byte & regAddr)                                                              
            digitalWrite(mdioPin, 1);                                                   
        else                                                                         
            digitalWrite(mdioPin, 0);                                                   
                                                                                     
        pulse_mdc();                                                                 
                                                                                     
        byte=byte>>1;                                                                
    }                                                                                
    /* Turn around bits */                                                           
    /* MDIO now is input */                                                          
    pinMode(mdioPin, INPUT);                                                            
    pulse_mdc();                                                                     
    pulse_mdc();                                                                     
    /* Data - 16 bits */                                                             
    data = 0;                                                                        
    for(word=0x8000;word!=0;){                                                       
                                                                                     
        if (digitalRead(mdioPin))                                                       
            data |= word;                                                            
                                                                                     
        pulse_mdc();
                                                                         
                                                                                     
        word=word>>1;                                                                
    }                                                                                
                                                                                     
    /* This is needed for some reason... */                                          
    pulse_mdc();                                                                     
    /* Stay in 0 state */                                                            
//  MDCIP = 0;                                                                       
                                                                                     
    return data;   
}

void MdcMdioController::writeMdc(uint8_t phyAddr, uint8_t regAddr, uint16_t data) {
    uint8_t byte;                                                                    
    uint16_t word;                                                                   
                                                                                     
    //void (*pulse_mdc)(void);                                                         
    //pulse_mdc;                                                         
                                                                                     
    /* MDIO pin is output */                                                         
    pinMode(mdioPin, OUTPUT);                                                           
                                                                                     
    digitalWrite(mdioPin, 1);                                                           
    for (byte = 0;byte < 32; byte++)                                                 
       pulse_mdc();                                                                 

    // // preamble
    // pulse_mdc(); 
    // digitalWrite(mdioPin, 1);
    // pulse_mdc(); 


    /* Stat code */                                                                  
    digitalWrite(mdioPin, 0);                                                           
    pulse_mdc();                                                                     
    digitalWrite(mdioPin, 1);                                                           
    pulse_mdc();                                                                     
                                                                                     
    /* Write OP Code */                                                              
    digitalWrite(mdioPin, 0);                                                           
    pulse_mdc();                                                                     
    digitalWrite(mdioPin, 1);                                                           
    pulse_mdc();                                                                     
                                                                                     
    /* PHY address - 5 bits */                                                       
    for (byte=0x10;byte!=0;byte=byte>>1){                                            
        if (byte & phyAddr)                                                              
            digitalWrite(mdioPin, 1);                                                   
        else                                                                         
            digitalWrite(mdioPin, 0);                                                   
        pulse_mdc();                                                                 
    }                                                                                
    /* REG address - 5 bits */                                                       
    for (byte=0x10;byte!=0;byte=byte>>1){                                            
        if (byte & regAddr)                                                              
            digitalWrite(mdioPin, 1);                                                   
        else                                                                         
            digitalWrite(mdioPin, 0);                                                   
                                                                                     
        pulse_mdc();                                                                 
    }                                                                                
    /* Turn around bits */                                                           
    digitalWrite(mdioPin, 1);                                                           
    pulse_mdc();                                                                     
    digitalWrite(mdioPin, 0);                                                           
    pulse_mdc();                                                                     
                                                                                     
    /* Data - 16 bits */                                                             
    for(word=0x8000;word!=0;word=word>>1){                                           
        if (word & data)                                                             
            digitalWrite(mdioPin, 1);                                                   
        else                                                                         
            digitalWrite(mdioPin, 0);                                                   
                                                                                     
        pulse_mdc();                                                                 
    }
                                                                                     
    /* This is needed for some reason... */                                          
    pulse_mdc();             
} 