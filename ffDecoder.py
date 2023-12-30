class ffDecoder:
    length = 0
    ffpos = 0
    state = 1
    pos = 0
    summ = 0xff
    packet = bytearray()

    def ff_Decoder(self, pdata, f):
        bt = pdata
        if bt == 0xff:
            self.length = 0
            self.ffpos = 0
            self.state = 1
            self.pos = 0
            self.summ = 0xff
            self.packet = bytearray()
        else:
            if self.state == 0:
                print("ex1")
                return (False, True)
            self.pos += 1
            if (self.state == 4 or self.state == 5) and self.ffpos != 0 and self.ffpos == self.pos:
                if bt > self.length - self.pos:
                    self.state = 0
                    print("ex2")
                    return (False, True)
                tpos = bt
                bt = 0xff
                self.ffpos = tpos + self.pos
            if self.state == 4 and self.pos == self.length - 1:
                self.state = 5
            else:
                if self.state != 3:
                    self.summ += bt
            if self.state == 1:
                if (bt != 0x01):
                    self.state = 0
                    print("ex3")
                    return (False, True)
                self.state = 2
            elif self.state == 2:
                if bt < 5 or bt > 254:
                    self.state = 0
                    print("ex4")
                    return (False, True)
                self.length = bt
                self.state = 3
            elif self.state == 3:
                if bt > self.length - self.pos:
                    self.state = 0
                    print("ex5")
                    return (False, True)
                self.ffpos = self.pos + bt
                self.state = 4
            elif self.state == 4:
                self.packet.append(bt)
            elif self.state == 5:
                if self.summ & 0xff != bt:
                    self.state = 0
                    print("ex6")
                    return (False, True)
                # packets.append(self.packet)
                # fileWrite(self.packet, f)
                self.state = 0
                return (True, self.packet)

        return (False, False)
