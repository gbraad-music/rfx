/**
 * VU Meter - Canvas Renderer for Web
 * EXTRACTED from effects.js - Preserves existing working implementation
 *
 * This wraps the existing drawVUMeter() function into a reusable component class.
 * All visual details, smoothing constants, and REGROOVE branding preserved exactly.
 *
 * Usage:
 *   const vuMeter = new VUMeterCanvas('vumeter-canvas');
 *
 *   // In audio callback:
 *   vuMeter.update(leftPeak, rightPeak);
 *
 *   // In animation loop:
 *   vuMeter.draw();
 *
 * Copyright (C) 2025
 * SPDX-License-Identifier: ISC
 */

class VUMeterCanvas {
    constructor(canvasId) {
        this.canvas = document.getElementById(canvasId);
        if (!this.canvas) {
            throw new Error(`Canvas element '${canvasId}' not found`);
        }

        this.ctx = this.canvas.getContext('2d');

        // VU meter state (from effects.js lines 1234-1239)
        this.vuLeftPeak = 0;
        this.vuRightPeak = 0;
        this.vuLeftSmoothed = 0;
        this.vuRightSmoothed = 0;

        // Constants EXACTLY as in effects.js
        this.VU_DECAY_RATE = 0.95;  // Slower decay (more dampening)
        this.VU_SMOOTHING = 0.7;    // Heavy smoothing on incoming values
    }

    /**
     * Update VU meter with new peak values
     * @param {number} leftPeak - Left channel peak (0.0-1.0)
     * @param {number} rightPeak - Right channel peak (0.0-1.0)
     */
    update(leftPeak, rightPeak) {
        // Convert to dB and map to needle position (from effects.js lines 1532-1545)
        const peakToDb = (peak) => {
            if (peak < 0.00001) return -100; // Silence
            return 20 * Math.log10(peak);
        };

        const dbToNeedle = (db) => {
            // Map -20dB to 0.0 (bottom), 0dBFS to 1.0 (top/RED)
            return Math.max(0, Math.min(1, (db + 20) / 20));
        };

        const leftDb = peakToDb(leftPeak);
        const rightDb = peakToDb(rightPeak);
        const leftNeedle = dbToNeedle(leftDb);
        const rightNeedle = dbToNeedle(rightDb);

        // Apply exponential smoothing to incoming values first (lines 1548-1549)
        this.vuLeftSmoothed = this.vuLeftSmoothed * this.VU_SMOOTHING + leftNeedle * (1 - this.VU_SMOOTHING);
        this.vuRightSmoothed = this.vuRightSmoothed * this.VU_SMOOTHING + rightNeedle * (1 - this.VU_SMOOTHING);

        // Peak hold with decay on smoothed values (lines 1552-1553)
        this.vuLeftPeak = Math.max(this.vuLeftSmoothed, this.vuLeftPeak * this.VU_DECAY_RATE);
        this.vuRightPeak = Math.max(this.vuRightSmoothed, this.vuRightPeak * this.VU_DECAY_RATE);
    }

    /**
     * Draw the VU meter - EXACT code from effects.js drawVUMeter() lines 1518-1691
     */
    draw() {
        const ctx = this.ctx;
        const width = this.canvas.width;
        const height = this.canvas.height;

        // Clear canvas (line 1556)
        ctx.fillStyle = '#0a0a0a';
        ctx.fillRect(0, 0, width, height);

        // Dimensions (lines 1560-1562)
        const pivotY = height / 2;  // Center vertically
        const arcRadius = Math.min(width * 0.45, height * 0.6);  // Bigger arc for taller meters
        const needleLength = arcRadius * 0.85;  // Needle shorter than arc

        // ===== LEFT METER (pivot on left edge) =====
        const leftPivotX = width * 0.05;

        // Draw left arc scale - from +90° to -90° counterclockwise (bottom, through right, to top) - INWARD
        ctx.strokeStyle = '#333';
        ctx.lineWidth = 3;
        ctx.beginPath();
        ctx.arc(leftPivotX, pivotY, arcRadius, Math.PI / 2, -Math.PI / 2, true);
        ctx.stroke();

        // Scale marks for left (lines 1574-1590)
        ctx.lineWidth = 1.5;
        for (let i = 0; i <= 10; i++) {
            const angle = Math.PI / 2 - (Math.PI * i / 10);
            const x1 = leftPivotX + arcRadius * Math.cos(angle);
            const y1 = pivotY + arcRadius * Math.sin(angle);
            const x2 = leftPivotX + (arcRadius - 8) * Math.cos(angle);
            const y2 = pivotY + (arcRadius - 8) * Math.sin(angle);

            // Red zone for top ticks (last 20%) - REGROOVE RED
            ctx.strokeStyle = i >= 8 ? '#CF1A37' : '#666';

            ctx.beginPath();
            ctx.moveTo(x1, y1);
            ctx.lineTo(x2, y2);
            ctx.stroke();
        }

        // LEFT needle: rests at +90° (bottom), swings toward -90°/270° (top)
        const leftAngle = Math.PI / 2 - this.vuLeftPeak * Math.PI;
        ctx.save();
        ctx.translate(leftPivotX, pivotY);
        ctx.rotate(leftAngle);

        // Needle shadow (lines 1598-1604)
        ctx.strokeStyle = 'rgba(207, 26, 55, 0.3)';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(needleLength, 0);
        ctx.stroke();

        // Needle - REGROOVE RED (lines 1606-1612)
        ctx.strokeStyle = '#CF1A37';
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(needleLength, 0);
        ctx.stroke();

        // Needle tip (lines 1614-1618)
        ctx.fillStyle = '#CF1A37';
        ctx.beginPath();
        ctx.arc(needleLength, 0, 2, 0, Math.PI * 2);
        ctx.fill();

        ctx.restore();

        // Pivot point (lines 1622-1626)
        ctx.fillStyle = '#666';
        ctx.beginPath();
        ctx.arc(leftPivotX, pivotY, 6, 0, Math.PI * 2);
        ctx.fill();

        // ===== RIGHT METER (pivot on right edge) =====
        const rightPivotX = width * 0.95;

        // Draw right arc scale - from +90° to +270° (bottom, through left, to top) - INWARD
        ctx.strokeStyle = '#333';
        ctx.lineWidth = 3;
        ctx.beginPath();
        ctx.arc(rightPivotX, pivotY, arcRadius, Math.PI / 2, Math.PI * 3 / 2);
        ctx.stroke();

        // Scale marks for right (lines 1638-1654)
        ctx.lineWidth = 1.5;
        for (let i = 0; i <= 10; i++) {
            const angle = Math.PI / 2 + (Math.PI * i / 10);
            const x1 = rightPivotX + arcRadius * Math.cos(angle);
            const y1 = pivotY + arcRadius * Math.sin(angle);
            const x2 = rightPivotX + (arcRadius - 8) * Math.cos(angle);
            const y2 = pivotY + (arcRadius - 8) * Math.sin(angle);

            // Red zone for top ticks (last 20%) - REGROOVE RED
            ctx.strokeStyle = i >= 8 ? '#CF1A37' : '#666';

            ctx.beginPath();
            ctx.moveTo(x1, y1);
            ctx.lineTo(x2, y2);
            ctx.stroke();
        }

        // RIGHT needle: rests at +90° (bottom), swings toward +270° (top)
        const rightAngle = Math.PI / 2 + this.vuRightPeak * Math.PI;
        ctx.save();
        ctx.translate(rightPivotX, pivotY);
        ctx.rotate(rightAngle);

        // Needle shadow (lines 1662-1668)
        ctx.strokeStyle = 'rgba(207, 26, 55, 0.3)';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(needleLength, 0);
        ctx.stroke();

        // Needle - REGROOVE RED (lines 1670-1676)
        ctx.strokeStyle = '#CF1A37';
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(needleLength, 0);
        ctx.stroke();

        // Needle tip (lines 1678-1682)
        ctx.fillStyle = '#CF1A37';
        ctx.beginPath();
        ctx.arc(needleLength, 0, 2, 0, Math.PI * 2);
        ctx.fill();

        ctx.restore();

        // Pivot point (lines 1686-1690)
        ctx.fillStyle = '#666';
        ctx.beginPath();
        ctx.arc(rightPivotX, pivotY, 6, 0, Math.PI * 2);
        ctx.fill();
    }

    /**
     * Reset meter state
     */
    reset() {
        this.vuLeftPeak = 0;
        this.vuRightPeak = 0;
        this.vuLeftSmoothed = 0;
        this.vuRightSmoothed = 0;
    }
}

// Export for use in other modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = VUMeterCanvas;
}
