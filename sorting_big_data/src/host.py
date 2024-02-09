import os
import sys
import uuid
import re

# Following found in PYTHONPATH setup by XRT
import pyxrt

from utils_binding import *

import smtplib
from email.mime.text import MIMEText


def send_email(subject, body, sender, recipients, password):
    msg = MIMEText(body)
    msg['Subject'] = subject
    msg['From'] = sender
    msg['To'] = ', '.join(recipients)
    with smtplib.SMTP_SSL('smtp.gmail.com', 465) as smtp_server:
        smtp_server.login(sender, password)
        smtp_server.sendmail(sender, recipients, msg.as_string())
    print("Message sent!")


def runKernel(opt):
    d = pyxrt.device(opt.index)
    xbin = pyxrt.xclbin(opt.bitstreamFile)
    uuid = d.load_xclbin(xbin)

    kernellist = xbin.get_kernels()

    rule = re.compile("vadd*")
    kernel = list(filter(lambda val: rule.match(val.get_name()), kernellist))[0]
    kHandle = pyxrt.kernel(d, uuid, kernel.get_name(), pyxrt.kernel.shared)
    subject = "Email Subject"
    recipients = "zahrachica2@gmail.com"
    sender = "rabetzohra@gmail.com"
    password = "fcnn wzri drnx wuce"
    zeros = bytearray(opt.DATA_SIZE)
    print("Allocate and initialize buffers")
    boHandle1 = pyxrt.bo(d, opt.DATA_SIZE, pyxrt.bo.normal, kHandle.group_id(0))
    boHandle1.write(zeros, 0)
    bo1 = boHandle1.map()

  #  boHandle2 = pyxrt.bo(d, opt.DATA_SIZE, pyxrt.bo.normal, kHandle.group_id(2))
   # boHandle2.write(zeros, 0)
   # bo2 = boHandle2.map()
    print("Allocate and initialize buffers out")
    boHandle3 = pyxrt.bo(d, opt.DATA_SIZE, pyxrt.bo.normal, kHandle.group_id(1))
    print("initialize with 0 ")
    boHandle3.write(zeros, 0)
    print("le map")
    bo3 = boHandle3.map()
    print("bo1 data")
    for i in range(opt.DATA_SIZE):
        bo1[i] = 1
    #    bo2[i] = 2
        #bufReference[i]=i
    print("pref")
    bufReference = [1 for i in range(opt.DATA_SIZE)]
    print("Allocate and initialize synchronize")
    boHandle1.sync(pyxrt.xclBOSyncDirection.XCL_BO_SYNC_BO_TO_DEVICE, opt.DATA_SIZE, 0)
    #boHandle2.sync(pyxrt.xclBOSyncDirection.XCL_BO_SYNC_BO_TO_DEVICE, opt.DATA_SIZE, 0)

    print("Start the kernel")
    run = kHandle(boHandle1, boHandle3,0, opt.DATA_SIZE)
    print("Now wait for the kernel to finish")
    state = run.wait()

    print("Get the output data from the device and validate it")
    boHandle3.sync(pyxrt.xclBOSyncDirection.XCL_BO_SYNC_BO_FROM_DEVICE, opt.DATA_SIZE, 0)

    for i in range(opt.DATA_SIZE):
        if bufReference[i] != bo3[i]:
            body = "Computed value done not match reference"
            # body = "Computed value done not match reference"
            send_email(subject, body, sender, recipients, password)
            print("Computed value done not match reference")
            assert False
        else:
            body = "Computed value done  match reference"
    send_email(subject, body, sender, recipients, password)


def main(args):
    opt = Options()
    Options.getOptions(opt, args)

    try:
        runKernel(opt)
        print("TEST PASSED")
        return 0

    except OSError as o:
        print(o)
        print("TEST FAILED")
        return -o.errno

    except AssertionError as a:
        print(a)
        print("TEST FAILED")
        return -1
    except Exception as e:
        print(e)
        print("TEST FAILED")
        return -1


if __name__ == "__main__":
    os.environ["Runtime.xrt_bo"] = "false"

    result = main(sys.argv)
    sys.exit(result)
