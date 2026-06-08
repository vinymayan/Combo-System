import { onCleanup, onMount } from 'solid-js';
import Konva from 'konva';

type TierVisualConfig = {
    letterColor: [number, number, number, number];
    backgroundColor: [number, number, number, number];
    imagePath: string;
};

type UiConfig = {
    enabled: boolean;
    showFloatingMessages: boolean;
    editMode: boolean;
    useTierImages: boolean;
    useTextProgressFill: boolean;
    showTierName: boolean;
    positionXPercent: number;
    positionYPercent: number;
    scalePercent: number;
    progressBarWidth: number;
    progressBarHeight: number;
    tiers: TierVisualConfig[];
};

type UiBridge = {
    updateComboMeter: (payload: string) => void;
    showComboMessage: (payload: string) => void;
    setComboTimerPaused: (payload: string) => void;
    updateComboUiSettings: (payload: string) => void;
};

const ranks = ['F', 'E', 'D', 'C', 'B', 'A', 'S', 'SS', 'SSS', 'Z'];
const rankNames = ['Feral', 'Eerie', 'Dismal', 'Crazy', 'Badass', 'Apocalyptic', 'Savage!', 'Sick Skills!', "Smokin' Style!", 'Zenith!'];
const defaultTierColors: [number, number, number, number][] = [
    [0.87, 0.91, 0.96, 1],
    [0.87, 0.91, 0.96, 1],
    [0.87, 0.91, 0.96, 1],
    [0.66, 0.84, 1.0, 1],
    [0.56, 0.78, 1.0, 1],
    [0.44, 0.72, 1.0, 1],
    [1.0, 0.85, 0.16, 1],
    [1.0, 0.88, 0.23, 1],
    [1.0, 0.94, 0.3, 1],
    [1.0, 0.94, 0.42, 1],
];
const defaultBackgroundColor: [number, number, number, number] = [0.02, 0.02, 0.02, 0.55];

const defaultConfig = (): UiConfig => ({
    enabled: true,
    showFloatingMessages: true,
    editMode: false,
    useTierImages: false,
    useTextProgressFill: false,
    showTierName: true,
    positionXPercent: 83,
    positionYPercent: 76,
    scalePercent: 200,
    progressBarWidth: 220,
    progressBarHeight: 13,
    tiers: defaultTierColors.map((letterColor) => ({
        letterColor,
        backgroundColor: defaultBackgroundColor,
        imagePath: '',
    })),
});

let bridge: UiBridge | null = null;
let pendingComboPayload: string | null = null;
let pendingSettingsPayload: string | null = null;
const pendingMessages: string[] = [];

(window as any).updateComboMeter = (payload: string) => {
    if (bridge) bridge.updateComboMeter(payload);
    else pendingComboPayload = payload;
};

(window as any).showComboMessage = (payload: string) => {
    if (bridge) bridge.showComboMessage(payload);
    else if (pendingMessages.length < 8) pendingMessages.push(payload);
};

(window as any).setComboTimerPaused = (payload: string) => {
    if (bridge) bridge.setComboTimerPaused(payload);
};

(window as any).updateComboUiSettings = (payload: string) => {
    if (bridge) bridge.updateComboUiSettings(payload);
    else pendingSettingsPayload = payload;
};

const clamp = (value: number, min: number, max: number) => Math.max(min, Math.min(max, value));

const colorToCss = (color: [number, number, number, number]) => {
    const r = Math.round(clamp(color[0], 0, 1) * 255);
    const g = Math.round(clamp(color[1], 0, 1) * 255);
    const b = Math.round(clamp(color[2], 0, 1) * 255);
    const a = clamp(color[3], 0, 1);
    return `rgba(${r},${g},${b},${a})`;
};

const normalizeImageSrc = (path: string) => {
    const trimmed = path.trim();
    if (!trimmed) return '';
    if (/^(https?:|file:|data:)/i.test(trimmed)) return trimmed;

    const normalized = trimmed.replace(/\\/g, '/');
    if (/^[a-z]:\//i.test(normalized)) {
        return `file:///${normalized}`;
    }
    return normalized;
};

function App() {
    let containerRef: HTMLDivElement | undefined;

    onMount(() => {
        if (!containerRef) return;

        const stage = new Konva.Stage({
            container: containerRef,
            width: window.innerWidth,
            height: window.innerHeight,
        });

        const staticLayer = new Konva.Layer({ listening: false });
        const dynamicLayer = new Konva.Layer({ listening: false });
        const staticGroup = new Konva.Group({ listening: false });
        const dynamicGroup = new Konva.Group({ listening: false });

        staticLayer.add(staticGroup);
        dynamicLayer.add(dynamicGroup);
        stage.add(staticLayer);
        stage.add(dynamicLayer);

        const barTrack = new Konva.Rect({
            x: 18,
            y: 126,
            width: 220,
            height: 13,
            fill: 'rgba(5,7,9,0.74)',
            cornerRadius: 4,
            listening: false,
        });
        const barInset = new Konva.Rect({
            x: 21,
            y: 129,
            width: 214,
            height: 7,
            fill: 'rgba(12,17,22,0.88)',
            cornerRadius: 3,
            listening: false,
        });
        staticGroup.add(barTrack, barInset);

        const rankVisualGroup = new Konva.Group({ listening: false });
        const rankShapeState = {
            text: '',
            fill: '#dfe9f4',
            baseFill: 'rgba(245,248,252,0.18)',
            progress: 0,
            useTextProgress: false,
        };
        const rankShape = new Konva.Shape({
            x: -18,
            y: -34,
            width: 170,
            height: 168,
            listening: false,
            sceneFunc: (context) => {
                const nativeContext = (context as any)._context as CanvasRenderingContext2D;
                const text = rankShapeState.text;
                if (!text) return;

                const fontSize = 104;
                const boxWidth = 164;
                const boxHeight = 150;
                const textTop = 12;
                nativeContext.save();
                nativeContext.font = `bold ${fontSize}px Futura, Arial, sans-serif`;
                nativeContext.textBaseline = 'top';

                const metrics = nativeContext.measureText(text);
                const x = Math.max(0, (boxWidth - metrics.width) * 0.5);

                const fill = (style: string) => {
                    nativeContext.fillStyle = style;
                    nativeContext.fillText(text, x, textTop);
                };

                fill(rankShapeState.useTextProgress ? rankShapeState.baseFill : rankShapeState.fill);

                if (rankShapeState.useTextProgress) {
                    const fillHeight = boxHeight * clamp(rankShapeState.progress, 0, 1);
                    nativeContext.save();
                    nativeContext.beginPath();
                    nativeContext.rect(0, boxHeight - fillHeight, boxWidth, fillHeight + 8);
                    nativeContext.clip();
                    fill(rankShapeState.fill);
                    nativeContext.restore();
                }

                nativeContext.restore();
            },
        });
        const imagePlaceholder = document.createElement('img');
        const rankImage = new Konva.Image({
            x: 2,
            y: -18,
            width: 124,
            height: 124,
            image: imagePlaceholder,
            visible: false,
            listening: false,
        });
        rankVisualGroup.add(rankShape, rankImage);

        const descText = new Konva.Text({
            x: 112,
            y: 30,
            width: 170,
            text: '',
            fontSize: 30,
            fontFamily: 'Futura, Arial, sans-serif',
            fontStyle: 'bold italic',
            fill: '#dfe9f4',
            listening: false,
        });
        const hitLabel = new Konva.Text({
            x: 128,
            y: 78,
            width: 116,
            text: '',
            fontSize: 15,
            fontFamily: 'Futura, Arial, sans-serif',
            fontStyle: 'bold',
            fill: '#d2e8ff',
            align: 'right',
            letterSpacing: 1,
            listening: false,
        });
        const barFill = new Konva.Rect({
            x: 21,
            y: 129,
            width: 0,
            height: 7,
            fill: '#81a4c7',
            cornerRadius: 3,
            listening: false,
        });

        dynamicGroup.add(rankVisualGroup, descText, hitLabel, barFill);

        type FloatingMessage = {
            group: Konva.Group;
            text: Konva.Text;
            active: boolean;
            start: number;
            duration: number;
            startX: number;
            startY: number;
        };

        const messagePool: FloatingMessage[] = [];
        for (let i = 0; i < 10; i++) {
            const group = new Konva.Group({ visible: false, listening: false });
            const text = new Konva.Text({
                width: 150,
                text: '',
                fontSize: 17,
                fontFamily: 'Futura, Arial, sans-serif',
                fontStyle: 'bold italic',
                fill: '#eaf6ff',
                align: 'center',
                listening: false,
            });
            group.add(text);
            dynamicGroup.add(group);
            messagePool.push({ group, text, active: false, start: 0, duration: 820, startX: 0, startY: 0 });
        }

        const imageCache = new Map<string, HTMLImageElement | null>();
        let config = defaultConfig();
        let isTimerPaused = false;
        let animationFrameId = 0;
        let lastFrameTime = performance.now();
        let actualHitValue = 0;
        let actualTier = 0;
        let actualPoints = 0;
        let actualPointsRequired = 100;
        let renderedHitValue = 0;
        let renderedTier = 0;
        let targetBarWidth = 0;
        let targetTextFillProgress = 0;

        const getTierConfig = (tier: number) => config.tiers[clamp(tier, 0, ranks.length - 1)] ?? defaultConfig().tiers[0];
        const usesTextProgress = () => config.useTextProgressFill && !config.useTierImages;
        const getUiScale = () => clamp(config.scalePercent, 40, 300) / 100;
        const getCachePixelRatio = () => clamp(getUiScale(), 1, 2.5);
        const getBarWidth = () => clamp(config.progressBarWidth, 60, 600);
        const getBarHeight = () => clamp(config.progressBarHeight, 6, 60);
        const getBarFillWidth = () => Math.max(1, getBarWidth() - 6);
        const getBarFillHeight = () => Math.max(1, getBarHeight() - 6);

        const applyBarLayout = () => {
            barTrack.width(getBarWidth());
            barTrack.height(getBarHeight());
            barInset.width(getBarFillWidth());
            barInset.height(getBarFillHeight());
            barFill.height(getBarFillHeight());
            hitLabel.x(18 + getBarWidth() - 116);
        };

        const setHudVisible = (visible: boolean) => {
            const showBar = visible && !usesTextProgress();
            staticGroup.visible(showBar);
            barFill.visible(showBar);
            dynamicGroup.visible(visible);
            if (showBar) {
                staticGroup.clearCache();
                staticGroup.cache({ pixelRatio: getCachePixelRatio() });
            }
        };

        const positionHud = () => {
            const scale = getUiScale();
            const hudWidth = Math.max(282, getBarWidth() + 36) * scale;
            const hudHeight = 170 * scale;
            const rawX = window.innerWidth * (clamp(config.positionXPercent, 0, 100) / 100);
            const rawY = window.innerHeight * (clamp(config.positionYPercent, 0, 100) / 100);
            const x = clamp(rawX, 0, Math.max(0, window.innerWidth - hudWidth));
            const y = clamp(rawY, 0, Math.max(0, window.innerHeight - hudHeight));

            applyBarLayout();
            staticGroup.position({ x, y });
            dynamicGroup.position({ x, y });
            staticGroup.scale({ x: scale, y: scale });
            dynamicGroup.scale({ x: scale, y: scale });

            staticGroup.visible(dynamicGroup.visible() && !usesTextProgress());
            staticGroup.clearCache();
            if (staticGroup.visible()) {
                staticGroup.cache({ pixelRatio: getCachePixelRatio() });
            }
            staticLayer.batchDraw();
        };

        const cacheRank = () => {
            rankVisualGroup.clearCache();
            if ((renderedHitValue > 0 || config.editMode) && !usesTextProgress()) {
                rankVisualGroup.cache({ pixelRatio: getCachePixelRatio() });
            }
        };

        const loadImage = (path: string, tier: number) => {
            const src = normalizeImageSrc(path);
            if (!src) return null;
            if (imageCache.has(src)) return imageCache.get(src) ?? null;

            imageCache.set(src, null);
            const image = new Image();
            image.onload = () => {
                imageCache.set(src, image);
                if (renderedTier === tier) {
                    applyTierStyle();
                    dynamicLayer.batchDraw();
                }
            };
            image.onerror = () => {
                imageCache.set(src, null);
            };
            image.src = src;
            return null;
        };

        function applyTierStyle() {
            const tier = clamp(renderedTier, 0, ranks.length - 1);
            const tierConfig = getTierConfig(tier);
            const color = colorToCss(tierConfig.letterColor);
            const backgroundColor = colorToCss(tierConfig.backgroundColor);
            const imagePath = tierConfig.imagePath.trim();
            const image = config.useTierImages && imagePath ? loadImage(imagePath, tier) : null;

            rankVisualGroup.clearCache();
            rankShapeState.text = ranks[tier];
            rankShapeState.fill = color;
            rankShapeState.baseFill = backgroundColor;
            rankShapeState.useTextProgress = usesTextProgress();
            const hudWidth = Math.max(282, getBarWidth() + 36);
            const rankX = config.showTierName ? -18 : Math.max(0, (hudWidth - 170) * 0.5);
            rankShape.x(rankX);
            rankImage.x(rankX + 20);
            descText.text(rankNames[tier]);
            descText.visible(config.showTierName);
            descText.fill(color);
            hitLabel.fill(tier >= 6 ? '#ffe986' : '#d2e8ff');
            barTrack.fill(backgroundColor);
            barInset.fill(backgroundColor);
            barFill.fill(color);

            if (image) {
                rankImage.image(image);
                rankImage.visible(true);
                rankShape.visible(false);
            } else {
                rankImage.visible(false);
                rankShape.visible(true);
            }

            cacheRank();
        }

        const requestFrame = () => {
            if (animationFrameId === 0) {
                lastFrameTime = performance.now();
                animationFrameId = requestAnimationFrame(updateLoop);
            }
        };

        const hasActiveMessages = () => messagePool.some((message) => message.active);

        const clearCombo = () => {
            renderedHitValue = 0;
            renderedTier = 0;
            targetBarWidth = 0;
            targetTextFillProgress = 0;
            setHudVisible(false);
            rankVisualGroup.clearCache();
            rankShapeState.text = '';
            rankShapeState.progress = 0;
            rankImage.visible(false);
            descText.text('');
            hitLabel.text('');
            barFill.width(0);
            for (const message of messagePool) {
                message.active = false;
                message.group.visible(false);
            }
            staticLayer.batchDraw();
            dynamicLayer.batchDraw();
        };

        const renderCombo = (hitValue: number, tierValue: number, comboPoints: number, pointsRequired: number) => {
            renderedHitValue = hitValue;
            renderedTier = clamp(tierValue, 0, ranks.length - 1);
            const progress = clamp(comboPoints / Math.max(1, pointsRequired), 0, 1);
            targetBarWidth = getBarFillWidth() * progress;
            targetTextFillProgress = progress;
            setHudVisible(true);
            hitLabel.text(`${hitValue} HITS`);
            applyTierStyle();
            staticLayer.batchDraw();
            requestFrame();
        };

        const renderEditPreview = () => {
            if (actualHitValue > 0) {
                renderCombo(actualHitValue, actualTier, actualPoints, actualPointsRequired);
            } else {
                renderCombo(12, 6, 70, 100);
            }
        };

        function updateLoop() {
            animationFrameId = 0;
            const now = performance.now();
            const deltaTime = Math.min((now - lastFrameTime) / 1000, 0.1);
            lastFrameTime = now;
            let needsFrame = false;

            if (!isTimerPaused) {
                const currentWidth = barFill.width();
                const easing = Math.min(1, deltaTime * 14);
                const nextWidth = currentWidth + ((targetBarWidth - currentWidth) * easing);
                const finalWidth = Math.abs(nextWidth - targetBarWidth) < 0.4 ? targetBarWidth : nextWidth;
                if (Math.abs(finalWidth - currentWidth) > 0.01) {
                    barFill.width(finalWidth);
                    needsFrame = true;
                }

                const currentTextProgress = rankShapeState.progress;
                const nextTextProgress = currentTextProgress + ((targetTextFillProgress - currentTextProgress) * easing);
                const finalTextProgress = Math.abs(nextTextProgress - targetTextFillProgress) < 0.004 ? targetTextFillProgress : nextTextProgress;
                if (Math.abs(finalTextProgress - currentTextProgress) > 0.001) {
                    rankShapeState.progress = finalTextProgress;
                    needsFrame = true;
                }

                for (const message of messagePool) {
                    if (!message.active) continue;
                    const progress = Math.min(1, (now - message.start) / message.duration);
                    const lift = 54 * progress;
                    message.group.position({ x: message.startX, y: message.startY - lift });
                    message.group.opacity(1 - progress);
                    if (progress >= 1) {
                        message.active = false;
                        message.group.visible(false);
                    } else {
                        needsFrame = true;
                    }
                }
            }

            if (renderedHitValue > 0 || config.editMode || hasActiveMessages()) {
                dynamicLayer.batchDraw();
            }

            if (needsFrame) {
                requestFrame();
            }
        }

        const spawnMessage = (label: string, delta: number) => {
            if (!label || delta === 0) return;
            const slot = messagePool.find((message) => !message.active) ?? messagePool[0];
            const sign = delta > 0 ? '+' : '';

            slot.active = true;
            slot.start = performance.now();
            slot.startX = 60 + (Math.random() * 36);
            slot.startY = 14 + (Math.random() * 14);
            slot.text.text(`${label} ${sign}${delta}`);
            slot.text.fill(delta > 0 ? '#f7ec8a' : '#ff9f9f');
            slot.group.position({ x: slot.startX, y: slot.startY });
            slot.group.opacity(1);
            slot.group.visible(true);
            requestFrame();
        };

        const parseConfig = (payload: string) => {
            const parsed = JSON.parse(payload) as Partial<UiConfig>;
            const next = defaultConfig();
            next.enabled = parsed.enabled !== false;
            next.showFloatingMessages = parsed.showFloatingMessages !== false;
            next.editMode = Boolean(parsed.editMode);
            next.useTierImages = Boolean(parsed.useTierImages);
            next.useTextProgressFill = Boolean(parsed.useTextProgressFill);
            next.showTierName = parsed.showTierName !== false;
            next.positionXPercent = clamp(Number(parsed.positionXPercent ?? next.positionXPercent), 0, 100);
            next.positionYPercent = clamp(Number(parsed.positionYPercent ?? next.positionYPercent), 0, 100);
            next.scalePercent = clamp(Number(parsed.scalePercent ?? next.scalePercent), 40, 300);
            next.progressBarWidth = clamp(Number(parsed.progressBarWidth ?? next.progressBarWidth), 60, 600);
            next.progressBarHeight = clamp(Number(parsed.progressBarHeight ?? next.progressBarHeight), 6, 60);

            if (Array.isArray(parsed.tiers)) {
                for (let i = 0; i < Math.min(parsed.tiers.length, ranks.length); i++) {
                    const tier = parsed.tiers[i] as Partial<TierVisualConfig>;
                    if (Array.isArray(tier.letterColor) && tier.letterColor.length >= 3) {
                        next.tiers[i].letterColor = [
                            clamp(Number(tier.letterColor[0]), 0, 1),
                            clamp(Number(tier.letterColor[1]), 0, 1),
                            clamp(Number(tier.letterColor[2]), 0, 1),
                            clamp(Number(tier.letterColor[3] ?? 1), 0, 1),
                        ];
                    }
                    if (Array.isArray(tier.backgroundColor) && tier.backgroundColor.length >= 3) {
                        next.tiers[i].backgroundColor = [
                            clamp(Number(tier.backgroundColor[0]), 0, 1),
                            clamp(Number(tier.backgroundColor[1]), 0, 1),
                            clamp(Number(tier.backgroundColor[2]), 0, 1),
                            clamp(Number(tier.backgroundColor[3] ?? 1), 0, 1),
                        ];
                    }
                    if (typeof tier.imagePath === 'string') {
                        next.tiers[i].imagePath = tier.imagePath;
                    }
                }
            }
            return next;
        };

        bridge = {
            updateComboMeter: (payload: string) => {
                if (!payload) return;

                const parts = payload.split('|');
                if (parts.length < 2) return;
                if (!config.enabled) {
                    clearCombo();
                    return;
                }

                actualHitValue = parseInt(parts[0], 10) || 0;
                actualTier = clamp(parseInt(parts[1], 10) || 0, 0, ranks.length - 1);
                actualPoints = parseInt(parts[2] ?? '0', 10) || 0;
                actualPointsRequired = Math.max(1, parseInt(parts[3] ?? '100', 10) || 100);

                if (actualHitValue === 0) {
                    if (config.editMode) {
                        renderEditPreview();
                    } else {
                        clearCombo();
                    }
                    return;
                }

                renderCombo(actualHitValue, actualTier, actualPoints, actualPointsRequired);
            },
            showComboMessage: (payload: string) => {
                if (!config.enabled || !config.showFloatingMessages) return;
                if (!payload) return;
                const separator = payload.lastIndexOf('|');
                if (separator <= 0) return;
                const label = payload.slice(0, separator);
                const delta = parseInt(payload.slice(separator + 1), 10) || 0;
                spawnMessage(label, delta);
            },
            setComboTimerPaused: (payload: string) => {
                isTimerPaused = payload === 'true';
                lastFrameTime = performance.now();
                if (!isTimerPaused) requestFrame();
            },
            updateComboUiSettings: (payload: string) => {
                if (!payload) return;
                try {
                    config = parseConfig(payload);
                    positionHud();
                    if (!config.enabled) {
                        clearCombo();
                    } else if (config.editMode || actualHitValue > 0) {
                        renderEditPreview();
                    } else {
                        clearCombo();
                    }
                    dynamicLayer.batchDraw();
                } catch {
                    return;
                }
            },
        };

        setHudVisible(false);
        positionHud();
        staticLayer.batchDraw();
        dynamicLayer.batchDraw();

        if (pendingSettingsPayload) {
            bridge.updateComboUiSettings(pendingSettingsPayload);
            pendingSettingsPayload = null;
        }
        if (pendingComboPayload) {
            bridge.updateComboMeter(pendingComboPayload);
            pendingComboPayload = null;
        }
        while (pendingMessages.length > 0) {
            bridge.showComboMessage(pendingMessages.shift() ?? '');
        }

        const handleResize = () => {
            stage.width(window.innerWidth);
            stage.height(window.innerHeight);
            positionHud();
            cacheRank();
            dynamicLayer.batchDraw();
        };

        window.addEventListener('resize', handleResize);

        onCleanup(() => {
            window.removeEventListener('resize', handleResize);
            if (animationFrameId !== 0) {
                cancelAnimationFrame(animationFrameId);
            }
            bridge = null;
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
