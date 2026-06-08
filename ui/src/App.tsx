import { onMount, onCleanup } from 'solid-js';
import Konva from 'konva';

function App() {
    let containerRef: HTMLDivElement | undefined;

    onMount(() => {
        if (!containerRef) return;

        const stage = new Konva.Stage({
            container: containerRef,
            width: window.innerWidth,
            height: window.innerHeight,
        });

        const layer = new Konva.Layer();
        stage.add(layer);

        const getBasePosition = () => ({
            x: window.innerWidth - 260,
            y: window.innerHeight - 220,
        });

        const { x: posX, y: posY } = getBasePosition();

        const hitText = new Konva.Text({
            x: posX,
            y: posY,
            text: '',
            fontSize: 72,
            fontFamily: 'Impact, Arial Black, sans-serif',
            fill: '#ffffff',
            stroke: '#000000',
            strokeWidth: 4,
            align: 'center',
            width: 200,
        });

        const labelText = new Konva.Text({
            x: posX,
            y: posY + 75,
            text: '',
            fontSize: 18,
            fontFamily: 'Arial, sans-serif',
            fill: '#ffaa00',
            fontStyle: 'bold',
            align: 'center',
            width: 200,
            letterSpacing: 2,
        });

        const barBg = new Konva.Rect({
            x: posX,
            y: posY + 105,
            width: 200,
            height: 12,
            fill: '#1a1a1a',
            stroke: '#333333',
            strokeWidth: 2,
            cornerRadius: 6,
        });

        const barFill = new Konva.Rect({
            x: posX + 2,
            y: posY + 107,
            width: 0,
            height: 8,
            fillLinearGradientStartPoint: { x: 0, y: 0 },
            fillLinearGradientEndPoint: { x: 196, y: 0 },
            fillLinearGradientColorStops: [0, '#e65c00', 1, '#f9d423'],
            cornerRadius: 4,
        });

        layer.add(hitText);
        layer.add(labelText);
        layer.add(barBg);
        layer.add(barFill);
        layer.draw();

        let isTimerPaused = false;
        let animationFrameId: number;
        let lastFrameTime = performance.now();
        let currentHitValue = 0;
        let currentTier = 0;
        let currentPoints = 0;
        let currentPointsRequired = 100;
        let targetBarWidth = 0;

        const ranks = ['F', 'E', 'D', 'C', 'B', 'A', 'S', 'SS', 'SSS', 'Z'];

        const getStyleRank = (tier: number): string => {
            const rankIndex = Math.max(0, Math.min(ranks.length - 1, tier));
            return ranks[rankIndex];
        };

        const applyTierStyle = () => {
            const intensity = Math.min(currentTier / 9, 1);
            const newFontSize = 72 + (38 * intensity);
            const { y } = getBasePosition();

            hitText.fontSize(newFontSize);
            hitText.y(y - (newFontSize - 72) * 0.5);
            hitText.shadowEnabled(false);
            hitText.shadowBlur(0);
            hitText.shadowOpacity(0);
            hitText.shadowOffset({ x: 0, y: 0 });

            if (currentTier < 3) {
                hitText.fill('#ffffff');
                hitText.stroke('#000000');
                hitText.strokeWidth(4);
                labelText.fill('#ffaa00');
            } else if (currentTier < 6) {
                hitText.fill('#ffea00');
                hitText.stroke('#1a1a1a');
                hitText.strokeWidth(4);
                labelText.fill('#ffea00');
            } else if (currentTier < 9) {
                hitText.fill('#ff6600');
                hitText.stroke('#050505');
                hitText.strokeWidth(5);
                labelText.fill('#ff3300');
            } else {
                hitText.fill('#ff0022');
                hitText.stroke('#ffd700');
                hitText.strokeWidth(5);
                labelText.fill('#ffd700');
            }
        };

        const clearCombo = () => {
            currentHitValue = 0;
            currentTier = 0;
            currentPoints = 0;
            targetBarWidth = 0;
            hitText.text('');
            labelText.text('');
            barFill.width(0);
            hitText.scale({ x: 1, y: 1 });
            hitText.shadowEnabled(false);
            layer.batchDraw();
        };

        const updateLoop = () => {
            const now = performance.now();
            const deltaTime = Math.min((now - lastFrameTime) / 1000, 0.1);
            lastFrameTime = now;

            if (!isTimerPaused) {
                const currentWidth = barFill.width();
                const easing = Math.min(1, deltaTime * 12);
                const nextWidth = currentWidth + ((targetBarWidth - currentWidth) * easing);
                barFill.width(Math.abs(nextWidth - targetBarWidth) < 0.5 ? targetBarWidth : nextWidth);

                if (currentHitValue > 0) {
                    layer.batchDraw();
                }
            }

            animationFrameId = requestAnimationFrame(updateLoop);
        };
        animationFrameId = requestAnimationFrame(updateLoop);

        (window as any).updateComboMeter = (payload: string) => {
            if (!payload) return;

            const parts = payload.split('|');
            if (parts.length < 2) return;

            const hitValue = parseInt(parts[0], 10) || 0;
            const tierValue = parseInt(parts[1], 10) || 0;
            const comboPoints = parseInt(parts[2] ?? '0', 10) || 0;
            const pointsRequired = Math.max(1, parseInt(parts[3] ?? '100', 10) || 100);

            currentHitValue = hitValue;
            currentTier = tierValue;
            currentPoints = comboPoints;
            currentPointsRequired = pointsRequired;
            targetBarWidth = 196 * Math.max(0, Math.min(currentPoints / currentPointsRequired, 1));

            if (hitValue === 0) {
                clearCombo();
                return;
            }

            hitText.text(getStyleRank(currentTier));
            labelText.text(`${hitValue} HITS`);
            applyTierStyle();

            const pulseScale = 1.08 + (0.08 * Math.min(currentTier / 9, 1));
            hitText.scale({ x: pulseScale, y: pulseScale });
            layer.batchDraw();

            window.setTimeout(() => {
                hitText.scale({ x: 1, y: 1 });
                layer.batchDraw();
            }, 70);
        };

        (window as any).setComboTimerPaused = (payload: string) => {
            isTimerPaused = payload === 'true';
            lastFrameTime = performance.now();
        };

        const handleResize = () => {
            stage.width(window.innerWidth);
            stage.height(window.innerHeight);
            const { x, y } = getBasePosition();
            const intensity = Math.min(currentTier / 9, 1);
            const currentFontSize = 72 + (38 * intensity);

            hitText.x(x);
            hitText.y(y - (currentFontSize - 72) * 0.5);
            labelText.x(x);
            labelText.y(y + 75);
            barBg.x(x);
            barBg.y(y + 105);
            barFill.x(x + 2);
            barFill.y(y + 107);
            layer.batchDraw();
        };

        window.addEventListener('resize', handleResize);

        onCleanup(() => {
            window.removeEventListener('resize', handleResize);
            cancelAnimationFrame(animationFrameId);
            delete (window as any).updateComboMeter;
            delete (window as any).setComboTimerPaused;
            stage.destroy();
        });
    });

    return (
        <div
            ref={containerRef}
            style={{
                position: 'absolute',
                top: 0,
                left: 0,
                width: '100vw',
                height: '100vh',
                'pointer-events': 'none',
                overflow: 'hidden',
            }}
        />
    );
}

export default App;
