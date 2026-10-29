// SPDX-FileCopyrightText: 2026 Contributors to the Media eXchange Layer project.
// SPDX-License-Identifier: Apache-2.0

// a stall between the two clock reads that make up `D` makes `D`
// too small, so every grain of the run is written early after that

use std::time::{Duration, Instant};

use gst::prelude::*;
use gstreamer as gst;
use gstreamer_app as gst_app;
use mxl::Rational;

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
    let api = mxl::load_api(mxl::config::get_mxl_so_path()).unwrap();
    let instance = mxl::MxlInstance::new(api, &domain, "").unwrap();
    let rate = Rational {
        numerator: 48_000,
        denominator: 1,
    };

    for _ in 0..10 {
        let flow_id = uuid::Uuid::new_v4().to_string();
        let pipeline = gst::parse::launch(&format!(
            "appsrc name=src format=time \
               caps=audio/x-raw,format=F32LE,layout=interleaved,channels=1,channel-mask=(bitmask)0x1,rate=48000 \
             ! mxlsink name=sink flow-id={flow_id} domain={domain}"
        ))
        .unwrap()
        .downcast::<gst::Pipeline>()
        .unwrap();
        let appsrc = pipeline
            .by_name("src")
            .unwrap()
            .downcast::<gst_app::AppSrc>()
            .unwrap();
        let sink = pipeline.by_name("sink").unwrap();
        pipeline.set_state(gst::State::Playing).unwrap();

        let hog_sink = sink.clone();
        let hog = std::thread::spawn(move || {
            let until = Instant::now() + Duration::from_millis(100);
            while Instant::now() < until {
                {
                    let _lock = hog_sink.object_lock();
                    spin(Duration::from_millis(2));
                }
                spin(Duration::from_micros(20));
            }
        });

        let mut buffer = gst::Buffer::with_size(480 * 4).unwrap();
        buffer.get_mut().unwrap().set_pts(gst::ClockTime::ZERO);
        appsrc.push_buffer(buffer).unwrap();
        appsrc.end_of_stream().unwrap();
        pipeline
            .bus()
            .unwrap()
            .timed_pop_filtered(gst::ClockTime::from_seconds(5), &[gst::MessageType::Eos])
            .unwrap();
        hog.join().unwrap();

        let clock = pipeline.clock().unwrap();
        let before = clock.time().nseconds() as i128;
        let mxl_now = instance.get_time() as i128;
        let after = clock.time().nseconds() as i128;
        let measured = mxl_now - (before + after) / 2;

        // the head index is the end of the write, so the buffer starts 480 earlier
        let reader = instance
            .create_flow_reader(&flow_id)
            .unwrap()
            .to_samples_reader()
            .unwrap();
        let first = reader.get_runtime_info().unwrap().headIndex - 480;
        let used = instance.index_to_timestamp(first, &rate).unwrap() as i128
            - sink.base_time().unwrap().nseconds() as i128;
        println!(
            "sink D is {:.1} us smaller than measured",
            (measured - used) as f64 / 1000.0
        );

        drop(reader);
        pipeline.set_state(gst::State::Null).unwrap();
    }
    std::fs::remove_dir_all(&domain).unwrap();
}
