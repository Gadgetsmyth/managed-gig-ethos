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

    // set standard page section
    writeMdc(0x00, 0x1f, 0x0000);
// hardware bringup
    // set broadcast mode. ( so we don't have to write both phys)
    writeMdc(0x00, 0x16, 0x3201);

    // switch to the test page
    writeMdc(0x00, 0x1f, 0x2A30);

    writeMdcMask(0x00, 0x18, 0x0000, 0x0400); // clear bias bit for 1000BT distortion
    writeMdcMask(0x00, 0x05, 0x0c00, 0x0e00); // optimize pre-emphasis for 100basetx
    writeMdcMask(0x00, 0x08, 0x8000, 0x8000); // token ring enable

    // change to token ring page
    writeMdc(0x00, 0x1f, 0x52B5);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x0068); 
    writeMdc(0x00, 0x11, 0x8980); 
    writeMdc(0x00, 0x10, 0x8f90);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x0000); 
    writeMdc(0x00, 0x11, 0x0003); 
    writeMdc(0x00, 0x10, 0x8796);
    
    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x0050); 
    writeMdc(0x00, 0x11, 0x100f); 
    writeMdc(0x00, 0x10, 0x87fa);
    
    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x0012); 
    writeMdc(0x00, 0x11, 0xb002); 
    writeMdc(0x00, 0x10, 0x8f82);
    
    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x0000); 
    writeMdc(0x00, 0x11, 0x0004); 
    writeMdc(0x00, 0x10, 0x9686);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x00d2); 
    writeMdc(0x00, 0x11, 0xc46f); 
    writeMdc(0x00, 0x10, 0x968c);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x0000); 
    writeMdc(0x00, 0x11, 0x0620); 
    writeMdc(0x00, 0x10, 0x97a2);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x00ee); 
    writeMdc(0x00, 0x11, 0xffdd); 
    writeMdc(0x00, 0x10, 0x96a0);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x0007); 
    writeMdc(0x00, 0x11, 0x1448); 
    writeMdc(0x00, 0x10, 0x96a6);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x0013); 
    writeMdc(0x00, 0x11, 0x132f); 
    writeMdc(0x00, 0x10, 0x96a4);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x0000); 
    writeMdc(0x00, 0x11, 0x0000); 
    writeMdc(0x00, 0x10, 0x96a8);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x00c0); 
    writeMdc(0x00, 0x11, 0xa028); 
    writeMdc(0x00, 0x10, 0x8ffc);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x0091); 
    writeMdc(0x00, 0x11, 0xb06c); 
    writeMdc(0x00, 0x10, 0x8fe8);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x0004); 
    writeMdc(0x00, 0x11, 0x1600); 
    writeMdc(0x00, 0x10, 0x8fea);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x00ff); 
    writeMdc(0x00, 0x11, 0xfaff); 
    writeMdc(0x00, 0x10, 0x8f80);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x0090); 
    writeMdc(0x00, 0x11, 0x1809); 
    writeMdc(0x00, 0x10, 0x8fec);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x00b0); 
    writeMdc(0x00, 0x11, 0x1007); 
    writeMdc(0x00, 0x10, 0x8ffe);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x00ee); 
    writeMdc(0x00, 0x11, 0xff00); 
    writeMdc(0x00, 0x10, 0x96b0);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x0000); 
    writeMdc(0x00, 0x11, 0x7000); 
    writeMdc(0x00, 0x10, 0x96b2);

    // 18 (0x12) 17 (0x11) 16 (0x10) 
    writeMdc(0x00, 0x12, 0x0000); 
    writeMdc(0x00, 0x11, 0x0814); 
    writeMdc(0x00, 0x10, 0x96b4);

    // switch to the test page
    writeMdc(0x00, 0x1f, 0x2A30);

    writeMdcMask(0x00, 0x08, 0x0000, 0x8000); // token ring enable

    
    // set standard page section
    writeMdc(0x00, 0x1f, 0x0000);
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

    // TODO: APPLY PRODUCTION PATCH HERE!

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
    writeMdc(0x00, 0x14, 0x0041);
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

void MdcMdioController::writeMdcMask(uint8_t phyAddr, uint8_t regAddr, uint16_t data, uint16_t mask) {
    uint16_t currentVal = readMdc(phyAddr, regAddr);
    uint16_t valToWrite = (currentVal & ~mask) | (data & mask);
    writeMdc(phyAddr, regAddr, valToWrite);
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