<script lang="ts">
	import { FontAwesomeIcon } from '@fortawesome/svelte-fontawesome';
	import {
		faBookmark,
		faTrash,
		faICursor,
		faClone,
	} from '@fortawesome/free-solid-svg-icons';
	import type { Calculation } from './calculator';
	import type { History } from './history';
	import { trackEvent } from '../routes/tracking';

	export let calculation: Calculation;
	export let history: History;
	export let onselectcalculation: (input: string) => void;
	/** this event triggers on the onMouseDownEvent */
	export let onabouttoselect: () => void;

	let hostElement: HTMLElement;

	function decodeHTMLentities(inp: string) {
		const el = document.createElement('div');
		el.innerHTML = inp;
		return el.textContent;
	}

	let copyAnimated = false;
	function copyClick() {
		navigator.clipboard.writeText(
			decodeHTMLentities(calculation.output.replace(/<[^>]*>/g, ''))!,
		);
		copyAnimated = true;
		setTimeout(() => {
			copyAnimated = false;
		}, 1000);
		trackEvent('history', 'copy');
	}

	function deleteClick() {
		history.delete(calculation.id);
	}

	function bookMarkClick() {
		if (calculation.isBookmarked) {
			history.removeBookmark(calculation.id);
		} else {
			history.bookmark(calculation.id);
		}
	}

	function renameClick() {
		const title = prompt(
			'Enter a title for the calculation:',
			calculation.bookmarkName,
		);
		history.renameBookmark(calculation.id, title ?? undefined);
	}

	/** filter the touch points which belong to our HistoryItem */
	function filterTouchList(list: TouchList) {
		const touches: Touch[] = [].slice.call(list);
		return touches.filter((t) => {
			let elem = t.target as HTMLElement;
			if (elem === hostElement) return true;
			while (elem.parentElement) {
				elem = elem.parentElement;
				if (elem === hostElement) return true;
			}
			return false;
		});
	}

	/** returns the mean x-coordinates of all touch points in `list` */
	function getMeanXPos(list: TouchList) {
		const touches = filterTouchList(list);
		return touches.reduce((v, t) => v + t.screenX, 0) / touches.length;
	}

	/** the x-coordinate where the last touch movement started */
	let startX: number | undefined;
	/** how much the touch point got shifted along the x axis */
	let offsetX: number = 0;
	/** how much the swipeable widget should be shifted */
	let shiftX: number = 0;
	/** the size of a single swipeButton */
	const swipeButtonSize = 100;

	function touchstart(evt: TouchEvent) {
		// subtracting offsetX takes care of the shift in mean position when adding a new touch point
		startX = getMeanXPos(evt.touches) - offsetX;
	}

	// whether the swipe actions will get initiated when the touch points are lifted
	let copySwipeSelected = false;
	let deleteSwipeSelected = false;
	let bookmarkSwipeSelected = false;
	let renameSwipeSelected = false;

	function touchmove(evt: TouchEvent) {
		if (startX === undefined) return;
		if (!evt.cancelable) {
			// this touchmove event might result in scrolling, let's therefore abort swiping
			shiftX = 0;
		} else {
			offsetX = getMeanXPos(evt.touches) - startX;
			const direction = Math.sign(offsetX);
			const abs = Math.abs(offsetX);

			/** how far the element can be swiped */
			let swipeSize = swipeButtonSize;
			if (direction > 0) {
				swipeSize = 2 * swipeButtonSize;
			} else if (direction < 0) {
				if (calculation.isBookmarked) {
					swipeSize = 2 * swipeButtonSize;
				}
			}

			/** close to the end of the swipable distance
			 * let's fade swiping out with an exponential function */
			const expKicksInAfter = swipeSize - swipeButtonSize / 2;
			if (abs < expKicksInAfter) {
				shiftX = offsetX;
			} else {
				shiftX =
					direction *
					(expKicksInAfter +
						(1 - Math.exp(-(abs - expKicksInAfter) / swipeSize)) *
							(swipeSize - expKicksInAfter));
			}
		}

		deleteSwipeSelected = shiftX > (swipeButtonSize * 3) / 2;
		copySwipeSelected =
			shiftX > swipeButtonSize / 2 && !deleteSwipeSelected;
		renameSwipeSelected = shiftX < (-swipeButtonSize * 3) / 2;
		bookmarkSwipeSelected =
			shiftX < -swipeButtonSize / 2 && !renameSwipeSelected;
	}

	function touchend(evt: TouchEvent) {
		if (filterTouchList(evt.touches).length > 0) {
			// subtracting offsetX takes care of the shift in mean position when adding a new touch point
			startX = getMeanXPos(evt.touches) - offsetX;
			return;
		}
		if (copySwipeSelected) {
			copySwipeSelected = false;
			copyClick();
		}
		if (deleteSwipeSelected) {
			deleteSwipeSelected = false;
			deleteClick();
		}
		if (bookmarkSwipeSelected) {
			bookmarkSwipeSelected = false;
			bookMarkClick();
		}
		if (renameSwipeSelected) {
			renameSwipeSelected = false;
			renameClick();
		}
		shiftX = 0;
		offsetX = 0;
	}

	function touchcancel() {
		shiftX = 0;
		offsetX = 0;
	}
</script>

<div
	class="responseHost"
	bind:this={hostElement}
	on:touchstart={touchstart}
	on:touchmove={touchmove}
	on:touchend={touchend}
	on:touchcancel={touchcancel}
>
	<button
		class="response"
		on:mousedown={() => onabouttoselect()}
		on:click={() => {
			onselectcalculation(calculation.rawInput);
			trackEvent('history', 'select');
		}}
		style={`transform: translateX(${shiftX}px)`}
	>
		{#if calculation.bookmarkName}
			<div class="name">
				<FontAwesomeIcon icon={faBookmark} />
				{calculation.bookmarkName}
			</div>
		{/if}
		<div>
			{@html calculation.input}
		</div>
		<div class="output">
			{@html calculation.output}
		</div>
		{#if calculation.messages.length > 0}
			<div
				class="message"
				class:error={calculation.severity === 'error'}
				class:warning={calculation.severity === 'warning'}
				class:info={calculation.severity === 'info'}
			>
				{#each calculation.messages as message}
					<div>{message}</div>
				{/each}
			</div>
		{/if}
	</button>
	<div class="mouseActions" style={`transform: translateX(${shiftX}px)`}>
		<button
			title="Copy result"
			class="copyButton"
			class:animated={copyAnimated}
			on:click={copyClick}
		>
			<FontAwesomeIcon icon={faClone} />
		</button>
		<button title="Remove" class="deleteButton" on:click={deleteClick}>
			<FontAwesomeIcon icon={faTrash} />
		</button>
		{#if calculation.isBookmarked}
			<button title="Rename" class="renameButton" on:click={renameClick}>
				<FontAwesomeIcon icon={faICursor} />
			</button>
		{/if}
		<button
			title={calculation.isBookmarked ? 'Remove bookmark' : 'Bookmark'}
			class="bookmarkButton"
			class:active={calculation.isBookmarked}
			on:click={bookMarkClick}
		>
			<FontAwesomeIcon icon={faBookmark} />
		</button>
	</div>
	<div class="swipeContainer left">
		<div class="swipeAction copy" class:selected={copySwipeSelected}>
			<FontAwesomeIcon icon={faClone} />
		</div>
		<div class="swipeAction delete" class:selected={deleteSwipeSelected}>
			<FontAwesomeIcon icon={faTrash} />
		</div>
	</div>
	<div class="swipeContainer right">
		{#if calculation.isBookmarked}
			<div
				class="swipeAction rename"
				class:selected={renameSwipeSelected}
			>
				<FontAwesomeIcon icon={faICursor} />
			</div>
		{/if}
		<div
			class="swipeAction bookmark"
			class:selected={bookmarkSwipeSelected}
		>
			<FontAwesomeIcon icon={faBookmark} />
		</div>
	</div>
</div>

<style>
	.response {
		background: #2a3030;
		border-radius: 10px;
		padding: 5px 10px;
		text-align: left;
		cursor: pointer;
		overflow: hidden;
		position: relative;
		display: block;
		width: 100%;
		border: none;
		color: inherit;
		margin-bottom: 10px;
		z-index: 2;
		will-change: transform;
	}

	.responseHost {
		position: relative;
		overflow: hidden;
		touch-action: pan-y;
	}

	.swipeContainer {
		position: absolute;
		top: 1px;
		bottom: 11px;
		overflow: hidden;
		display: flex;
	}

	.swipeContainer.left {
		border-radius: 10px 0 0 10px;
		left: 1px;
	}

	.swipeContainer.right {
		border-radius: 0 10px 10px 0;
		right: 1px;
	}

	.swipeAction {
		display: flex;
		align-items: center;
		justify-content: center;
		height: 100%;
		width: 100px;
		font-size: 1.5em;
		color: rgba(255, 255, 255, 0.5);
	}

	.swipeAction :global(svg) {
		transition: transform 0.2s;
	}

	.swipeAction.selected {
		color: white;
	}

	.swipeAction.selected :global(svg) {
		transform: scale(1.2);
	}

	.swipeAction.copy {
		background: lightskyblue;
	}

	.swipeAction.delete {
		background: red;
	}

	.swipeAction.bookmark {
		background: orange;
	}

	.swipeAction.rename {
		background: lightgreen;
	}

	.mouseActions {
		position: absolute;
		right: 0;
		bottom: 10px;
		padding-left: 10px;
		cursor: default;
		z-index: 3;
	}

	.mouseActions button {
		width: 40px;
		height: 40px;
		padding: 0;
		border: none;
		background: transparent;
		cursor: pointer;
		color: #788;
		opacity: 0.3;
		transition: opacity 0.1s;
	}

	@media (hover: hover) {
		.mouseActions button {
			opacity: 0;
		}
	}

	@media (hover: none) {
		.mouseActions button:not(.active) {
			/* Unfortunately, this media query is still quite unreliable. Let's therefore NOT apply display: none; */
		}
	}

	:global(.isTouchScreen) .mouseActions button:not(.active) {
		display: none;
	}

	.responseHost:hover .mouseActions button {
		opacity: 1;
	}

	.mouseActions button.active,
	.mouseActions button:focus:focus-visible {
		opacity: 0.5;
	}

	.mouseActions button:last-child {
		border-bottom-right-radius: 10px;
	}

	.copyButton:hover {
		color: lightskyblue;
	}

	.copyButton.animated {
		animation: copyButtonAnimation 0.3s;
	}

	@keyframes copyButtonAnimation {
		0% {
			transform: scale(1);
		}
		50% {
			transform: scale(1.5);
		}
		100% {
			transform: scale(1);
		}
	}

	.deleteButton:hover {
		color: red;
	}

	.renameButton:hover {
		color: lightgreen;
	}

	.bookmarkButton:hover {
		color: orange;
	}

	.message {
		margin: 10px 0 5px;
		font-size: 0.8em;
		opacity: 0.8;
		-webkit-hyphens: auto;
		hyphens: auto;
	}

	.error {
		color: lightsalmon;
	}

	.warning {
		color: orange;
	}

	.info {
		opacity: 0.8;
	}

	.output {
		margin: 20px 40px;
		font-size: 1.1em;
	}

	.name {
		margin-bottom: 5px;
		color: #9aa;
		font-size: 0.9em;
	}

	.name :global(svg) {
		opacity: 0.6;
		margin-right: 7px;
		transform-origin: top left;
		transform: translateY(-10px) scale(1.5);
	}
</style>
