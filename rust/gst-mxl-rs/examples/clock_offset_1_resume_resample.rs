// SPDX-FileCopyrightText: 2026 Contributors to the Media eXchange Layer project.
// SPDX-License-Identifier: Apache-2.0

use std::time::{Duration, Instant};

use gst::glib;
use gst::prelude::*;
use gstreamer as gst;

fn spin(duration: Duration) {
    let until = Instant::now() + duration;
    while Instant::now() < until {}
}

fn main() {
    gst::init().unwrap();
    gst::Element::register(
        None,
        "mxlsink",
        gst::Rank::NONE,
        gstmxl::mxlsink::MxlSink::static_type(),
    )
    .unwrap();
    let domain = format!("/dev/shm/mxl_repro_{}", uuid::Uuid::new_v4());
    std::fs::create_dir_all(&domain).unwrap();

    let pipeline = gst::parse::launch(&format!(
        "audiotestsrc is-live=true samplesperbuffer=480 ! audioconvert \
         ! audio/x-raw,format=F32LE,channels=1,rate=48000 \
         ! mxlsink name=sink flow-id={} domain={domain}",
        uuid::Uuid::new_v4()
    ))
    .unwrap()
    .downcast::<gst::Pipeline>()
    .unwrap();
    let sink = pipeline.by_name("sink").unwrap();
    pipeline.set_state(gst::State::Playing).unwrap();

    std::thread::spawn(move || {
        loop {
            {
                let _lock = sink.object_lock();
                spin(Duration::from_millis(2));
            }
            spin(Duration::from_micros(20));
        }
    });

    let clocks: [gst::Clock; 2] = [
        glib::Object::new::<gst::SystemClock>().upcast(),
        glib::Object::new::<gst::SystemClock>().upcast(),
    ];
    let bus = pipeline.bus().unwrap();
    for i in 0..1000 {
        pipeline.set_state(gst::State::Paused).unwrap();
        pipeline.set_clock(Some(&clocks[i % 2])).unwrap();
        pipeline.set_state(gst::State::Playing).unwrap();
        std::thread::sleep(Duration::from_millis(30));
        if let Some(msg) = bus.pop_filtered(&[gst::MessageType::Error]) {
            println!("reproduced after {} resumes: {msg:?}", i + 1);
            break;
        }
    }
    std::fs::remove_dir_all(&domain).unwrap();
    std::process::exit(0);
}
