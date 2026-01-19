/**
 * Waveform Display - Canvas Renderer for Web
 * EXTRACTED from effects.js - Preserves existing working implementation
 *
 * This wraps the existing drawVisualizer() function into a reusable component class.
 * All visual details and REGROOVE branding preserved exactly.
 *
 * Usage:
 *   const waveform = new WaveformDisplayCanvas('visualizer-canvas');
 *
 *   // Get analyser data from your audio processor
 *   const dataArray = processor.getAnalyserData();
 *   waveform.draw(dataArray);
 *
 * Copyright (C) 2025
 * SPDX-License-Identifier: ISC
 */

class WaveformDisplayCanvas {
    constructor(canvasId) {
        this.canvas = document.getElementById(canvasId);
        if (!this.canvas) {
            throw new Error(`Canvas element '${canvasId}' not found`);
        }

        this.ctx = this.canvas.getContext('2d');
        this.animationId = null;
        this.isAnimating = false;
    }

    /**
     * Draw waveform from analyser data - EXACT code from effects.js lines 1740-1776
     * @param {Uint8Array|Float32Array} dataArray - Time domain data from analyser
     */
    draw(dataArray) {
        const ctx = this.ctx;
        const canvas = this.canvas;

        // Auto-resize canvas to match container (lines 1717-1718)
        canvas.width = canvas.offsetWidth;
        canvas.height = canvas.offsetHeight;

        const bufferLength = dataArray.length;

        // Background (line 1740-1741)
        ctx.fillStyle = '#0a0a0a';
        ctx.fillRect(0, 0, canvas.width, canvas.height);

        // Grid (lines 1743-1752)
        ctx.strokeStyle = '#1a1a1a';
        ctx.lineWidth = 1;
        for (let i = 0; i < 5; i++) {
            const y = (canvas.height / 4) * i;
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(canvas.width, y);
            ctx.stroke();
        }

        // Waveform - REGROOVE RED (lines 1754-1776)
        ctx.lineWidth = 2;
        ctx.strokeStyle = '#CF1A37';
        ctx.beginPath();

        const sliceWidth = canvas.width / bufferLength;
        let x = 0;

        for (let i = 0; i < bufferLength; i++) {
            const v = dataArray[i] / 128.0;
            const y = v * canvas.height / 2;

            if (i === 0) {
                ctx.moveTo(x, y);
            } else {
                ctx.lineTo(x, y);
            }

            x += sliceWidth;
        }

        ctx.lineTo(canvas.width, canvas.height / 2);
        ctx.stroke();
    }

    /**
     * Start continuous animation loop
     * @param {Function} getDataCallback - Function that returns analyser data array
     */
    startAnimation(getDataCallback) {
        if (this.isAnimating) return;

        this.isAnimating = true;

        const animate = () => {
            if (!this.isAnimating) return;

            this.animationId = requestAnimationFrame(animate);

            const dataArray = getDataCallback();
            if (dataArray) {
                this.draw(dataArray);
            }
        };

        animate();
    }

    /**
     * Stop animation loop
     */
    stopAnimation() {
        this.isAnimating = false;
        if (this.animationId) {
            cancelAnimationFrame(this.animationId);
            this.animationId = null;
        }
    }
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = WaveformDisplayCanvas;
}
