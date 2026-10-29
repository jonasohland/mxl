// SPDX-FileCopyrightText: 2026 Contributors to the Media eXchange Layer project.
// SPDX-License-Identifier: Apache-2.0

// two mxlsink elements of one pipeline that start on different threads
// can each create their own offset cell, so they no longer share `D`

use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, Barrier};

use gst::prelude::*;
use gstreamer as gst;

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

    let mut split = 0;
    for _ in 0..100 {
        let pipeline = gst::Pipeline::new();
        let sinks: Vec<gst::Element> = (0..2)
            .map(|_| {
                gst::ElementFactory::make("mxlsink")
                    .property("flow-id", uuid::Uuid::new_v4().to_string())
                    .property("domain", &domain)
                    .build()
                    .unwrap()
            })
            .collect();
        for sink in &sinks {
            pipeline.add(sink).unwrap();
            sink.set_locked_state(true);
        }
        let have_context = Arc::new(AtomicUsize::new(0));
        let counter = have_context.clone();
        pipeline.bus().unwrap().set_sync_handler(move |_, msg| {
            if let gst::MessageView::HaveContext(_) = msg.view() {
                counter.fetch_add(1, Ordering::SeqCst);
            }
            gst::BusSyncReply::Drop
        });

        pipeline.set_state(gst::State::Ready).unwrap();

        let barrier = Arc::new(Barrier::new(2));
        let threads: Vec<_> = sinks
            .iter()
            .cloned()
            .map(|sink| {
                let barrier = barrier.clone();
                std::thread::spawn(move || {
                    barrier.wait();
                    sink.set_state(gst::State::Ready).unwrap();
                })
            })
            .collect();
        for thread in threads {
            thread.join().unwrap();
        }
        if have_context.load(Ordering::SeqCst) > 1 {
            split += 1;
        }
        for sink in &sinks {
            sink.set_locked_state(false);
        }
        pipeline.set_state(gst::State::Null).unwrap();
    }
    println!("{split} of 100 pipelines created two cells");
    std::fs::remove_dir_all(&domain).unwrap();
}
