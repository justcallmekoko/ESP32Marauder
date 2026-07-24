/*
 * PCF85063 - Arduino/PlatformIO RTC library
 * RTClib RTC_PCF8523-compatible interface
 * Register map from Waveshare BSP PCF85063Constants.h
 */


#include "configs.h"

#ifdef HAS_PCF85063

#include "PCF85063.h"

PCF85063::PCF85063() : _wire(&Wire) {}

// -- RTClib-compatible begin(TwoWire*) -----------------------------------------
bool PCF85063::begin(TwoWire *wire) {
    _wire = wire ? wire : &Wire;
    return _initImpl();
}

// -- Extended begin(TwoWire&, sda, scl) - Waveshare BSP style -----------------
bool PCF85063::begin(TwoWire &wire, int sda, int scl) {
    _wire = &wire;
    if (sda != -1 && scl != -1) {
        _wire->setPins(sda, scl);
    }
    _wire->begin();
    return _initImpl();
}

// -- Shared init - mirrors SensorPCF85063::initImpl() -------------------------
bool PCF85063::_initImpl() {
    // Chip detection via RAM register R/W test
    int val = readReg(REG_RAM);
    if (val < 0) {
        log_e("PCF85063: not responding");
        return false;
    }
    uint8_t tmp = (uint8_t)val;

    writeReg(REG_RAM, tmp | 0x80);
    if (!(readReg(REG_RAM) & 0x80)) {
        log_e("PCF85063: RAM bit7 set failed - may be PCF8563");
        return false;
    }
    writeReg(REG_RAM, tmp & ~0x80);
    if (readReg(REG_RAM) & 0x80) {
        log_e("PCF85063: RAM bit7 clear failed");
        return false;
    }
    writeReg(REG_RAM, tmp);  // restore

    // Force 24H mode
    int ctrl1 = readReg(REG_CTRL1);
    if (ctrl1 < 0) return false;
    if (ctrl1 & (1 << BIT_12H)) {
        writeReg(REG_CTRL1, (uint8_t)(ctrl1 & ~(1 << BIT_12H)));
        log_d("PCF85063: forced 24H mode");
    }

    start();

    // begin() returns true = chip found and clock running
    // begin() returns false = chip not on bus or hardware fault
    // OS bit / time validity is the caller's responsibility via
    // initialized() / lostPower() - checked separately after begin()
    return isrunning();
}

// -- RTClib interface ----------------------------------------------------------

DateTime PCF85063::now() {
    uint8_t buf[7];
    if (!readRegs(REG_SEC, buf, 7)) return DateTime();

    return DateTime(
        (uint16_t)_bcd2dec(buf[6])          + 2000,  // YEAR  (REG_YEAR  = 0x0A, offset 6)
        _bcd2dec(buf[5] & 0x1F),                      // MONTH (REG_MONTH = 0x09, offset 5)
        _bcd2dec(buf[3] & 0x3F),                      // DAY   (REG_DAY   = 0x07, offset 3)
        _bcd2dec(buf[2] & 0x3F),                      // HOUR  (REG_HOUR  = 0x06, offset 2)
        _bcd2dec(buf[1] & 0x7F),                      // MIN   (REG_MIN   = 0x05, offset 1)
        _bcd2dec(buf[0] & 0x7F)                       // SEC   (REG_SEC   = 0x04, offset 0)
    );
}

void PCF85063::adjust(const DateTime &dt) {
    uint8_t buf[7];
    buf[0] = _dec2bcd(dt.second())          & 0x7F;  // SEC   - clear OS bit
    buf[1] = _dec2bcd(dt.minute());                   // MIN
    buf[2] = _dec2bcd(dt.hour());                     // HOUR
    buf[3] = _dec2bcd(dt.day());                      // DAY
    buf[4] = dt.dayOfTheWeek();                       // WEEKDAY
    buf[5] = _dec2bcd(dt.month());                    // MONTH
    buf[6] = _dec2bcd((uint8_t)(dt.year() % 100));   // YEAR

    _wire->beginTransmission(PCF85063_ADDR);
    _wire->write(REG_SEC);
    _wire->write(buf, 7);
    bool ok = (_wire->endTransmission() == 0);
    log_d("PCF85063::adjust %s  %04u-%02u-%02u %02u:%02u:%02u",
          ok ? "OK" : "FAIL",
          dt.year(), dt.month(), dt.day(),
          dt.hour(), dt.minute(), dt.second());
}

// OS bit clear = clock has been running continuously = initialized
bool PCF85063::initialized() {
    int val = readReg(REG_SEC);
    if (val < 0) return false;
    return !(val & (1 << BIT_OS));
}

// inverse of initialized() - mirrors RTClib lostPower()
bool PCF85063::lostPower() {
    return !initialized();
}

void PCF85063::start() {
    int val = readReg(REG_CTRL1);
    if (val >= 0) writeReg(REG_CTRL1, (uint8_t)(val & ~(1 << BIT_STOP)));
}

void PCF85063::stop() {
    int val = readReg(REG_CTRL1);
    if (val >= 0) writeReg(REG_CTRL1, (uint8_t)(val | (1 << BIT_STOP)));
}

uint8_t PCF85063::isrunning() {
    int val = readReg(REG_CTRL1);
    if (val < 0) return 0;
    return !(val & (1 << BIT_STOP));
}

// -- Low-level I2C -------------------------------------------------------------

bool PCF85063::writeReg(uint8_t reg, uint8_t value) {
    _wire->beginTransmission(PCF85063_ADDR);
    _wire->write(reg);
    _wire->write(value);
    return (_wire->endTransmission() == 0);
}

int PCF85063::readReg(uint8_t reg) {
    uint8_t value = 0;
    _wire->beginTransmission(PCF85063_ADDR);
    _wire->write(reg);
    _wire->endTransmission(true);
    _wire->requestFrom(PCF85063_ADDR, (uint8_t)1);
    if (_wire->readBytes(&value, 1) != 1) return -1;
    return value;
}

bool PCF85063::readRegs(uint8_t reg, uint8_t *buf, uint8_t len) {
    _wire->beginTransmission(PCF85063_ADDR);
    _wire->write(reg);
    _wire->endTransmission(true);
    _wire->requestFrom(PCF85063_ADDR, len);
    return (_wire->readBytes(buf, len) == len);
}

#endif   // HAS_PCF85063
